#cython: language_level=3

import array
from cpython cimport array
from libc.stdlib cimport calloc, free
from libc.string cimport strcpy, memcpy, strncpy
import logging
from logging.handlers import SysLogHandler
import importlib
import importlib.util
from multiprocessing import Process, SimpleQueue
import os
from pathlib import Path
import platform
import struct
import subprocess
import sys
import time
import warnings

cimport cython
import numpy as np
cimport numpy as np

from pippi import dsp, midi, ugens
from pippi.soundbuffer cimport SoundBuffer

NUM_COMRADES = 16

class InstrumentError(Exception):
    pass

logger = logging.getLogger('astrid-cyrenderer')
if not logger.handlers:
    if platform.system() == 'Darwin':
        log_path = '/var/run/syslog'
    else:
        log_path = '/dev/log'

    logger.addHandler(SysLogHandler(address=log_path))
    logger.setLevel(logging.DEBUG)
    warnings.simplefilter('always')


cdef bytes serialize_buffer(SoundBuffer buf, int is_looping, lpmsg_t msg):
    cdef bytearray strbuf
    cdef size_t audiosize, length, msgsize
    cdef int channels, samplerate
    cdef unsigned char[:] msgview

    msgsize = sizeof(lpmsg_t)

    channels = <int>buf.channels
    samplerate = <int>buf.samplerate
    length = <size_t>len(buf)

    # size of audio data 
    audiosize = length * channels * sizeof(lpfloat_t)

    strbuf = bytearray()

    strbuf += struct.pack('N', audiosize)
    strbuf += struct.pack('N', length)
    strbuf += struct.pack('i', channels)
    strbuf += struct.pack('i', samplerate)
    strbuf += struct.pack('i', is_looping)
    strbuf += struct.pack('N', 0)

    for i in range(length):
        for c in range(channels):
            strbuf += struct.pack('d', lpfilternan(buf.frames[i,c]))

    msgview = <unsigned char[:msgsize]>(<unsigned char *>&msg)
    for i in range(msgsize):
        strbuf.append(msgview[i])

    return bytes(strbuf)

cdef class MessageEvent:
    def __cinit__(self,
            double onset,
            str instrument_name,
            int msgtype,
            str params,
            double max_processing_time,
        ):
        cdef size_t onset_frames = 0

        logger.error('MessageEvent params: %s' % params)

        params_byte_string = params.encode('utf-8')
        instrument_name_byte_string = instrument_name.encode('utf-8')
        cdef char * byte_params = params_byte_string
        cdef char * byte_instrument_name = instrument_name_byte_string

        self.msg = <lpmsg_t *>calloc(1, sizeof(lpmsg_t))
        self.msg.initiated = 0
        self.msg.scheduled = onset
        self.msg.completed = 0
        self.msg.max_processing_time = max_processing_time
        self.msg.onset_delay = 0
        #self.msg.voice_id = astrid_get_voice_id()
        self.msg.voice_id = 0
        self.msg.count = 0
        self.msg.type = msgtype
        self.msg.flags = LPFLAG_IS_SCHEDULED if onset > 0 else LPFLAG_NONE

        strcpy(self.msg.msg, byte_params)
        strcpy(self.msg.instrument_name, byte_instrument_name)

    cpdef int schedule(MessageEvent self, double now=0):
        self.msg.initiated = now
        return send_play_message(self.msg[0])

    def __dealloc__(self):
        if self.msg is not NULL:
            free(self.msg)

cdef class MidiEvent:
    def __cinit__(self,
            double onset,
            double length,
            char type,
            char note,
            char velocity,
            char program,
            char bank_msb,
            char bank_lsb,
            char channel
        ):

        self.event = <lpmidievent_t *>calloc(1, sizeof(lpmidievent_t))
        self.event.onset = onset
        self.event.now = 0
        self.event.length = length
        self.event.type = type
        self.event.note = note
        self.event.velocity = velocity
        self.event.program = program
        self.event.bank_msb = bank_msb
        self.event.bank_lsb = bank_lsb
        self.event.channel = channel

    cpdef int schedule(MidiEvent self, double now):
        self.event.now = now
        cdef int qfd = midi_triggerq_open()
        if qfd < 0:
            logger.exception('Error opening MIDI fifo q')
            return -1

        if midi_triggerq_schedule(qfd, self.event[0]) < 0:
            logger.exception('Error scheduling MidiEvent')
            return -1

        if midi_triggerq_close(qfd) < 0:
            logger.exception('Error closing MIDI fifo q')
            return -1

        return 0 

    def __dealloc__(self):
        if self.event is not NULL:
            free(self.event)

cdef class MidiEventListenerProxy:
    def __cinit__(self, default_device_id=1):
        self.default_device_id = default_device_id

    cpdef float cc(self, int cc, int device_id=-1):
        if device_id < 0:
            device_id = self.default_device_id
        cdef int value = lpmidi_getcc(device_id, cc)
        return float(value) / 127

    cpdef int cci(self, int cc, int device_id=-1):
        if device_id < 0:
            device_id = self.default_device_id
        return lpmidi_getcc(device_id, cc)

    cpdef float note(self, int note, int device_id=-1):
        if device_id < 0:
            device_id = self.default_device_id
        cdef int velocity = lpmidi_getnote(device_id, note)
        return float(velocity) / 127

    cpdef int notei(self, int note, int device_id=-1):
        if device_id < 0:
            device_id = self.default_device_id
        return lpmidi_getnote(device_id, note)

cdef class SerialEventListenerProxy:
    cpdef lpfloat_t ctl(self, int ctl, int device_id=0):
        cdef lpfloat_t value = 0
        if lpserial_getctl(device_id, ctl, &value) < 0:
            return 0
        return value

cdef class EventTriggerFactory:
    def _parse_params(self, *args, **kwargs):
        params = ' '.join(map(str, args)) 
        params += ' ' 
        params += ' '.join([ '%s=%s' % (k, v) for k, v in kwargs.items() ])

        logger.error('_parse_params params=%s' % params)

        return params

    cpdef midi(self, double onset, double length, double freq=0, double amp=1, int note=60, int program=1, int bank_msb=0, int bank_lsb=0, int channel=1, int device=-1):
        cdef char _note, velocity
        cdef MidiEvent noteon
        cdef MidiEvent noteoff
        
        if freq > 0:
            _note = <char>midi.ftomi(freq)
        else:
            _note = <char>max(0, min(127, note))

        velocity = <char>max(0, min(127, (amp * 127)))

        noteon = MidiEvent(onset, length, <char>NOTE_ON, note, velocity, program, bank_msb, bank_lsb, <char>channel)
        noteoff = MidiEvent(onset + length, 0, <char>NOTE_OFF, note, velocity, program, bank_msb, bank_lsb, <char>channel)

        return [noteon, noteoff]

    def play(self, double onset, str instrument_name, *args, **kwargs):
        params = self._parse_params(*args, **kwargs)
        return MessageEvent(onset, instrument_name, LPMSG_PLAY, params, 0)

    def trigger(self, double onset, str instrument_name, *args, **kwargs):
        params = self._parse_params(*args, **kwargs)
        return MessageEvent(onset, instrument_name, LPMSG_TRIGGER, params, 0)

    def update(self, double onset, str instrument_name, *args, **kwargs):
        params = self._parse_params(*args, **kwargs)
        return MessageEvent(onset, instrument_name, LPMSG_UPDATE, params, 0)

    def serial(self, double onset, str tty, *args, **kwargs):
        params = self._parse_params(*args, **kwargs)
        return MessageEvent(onset, tty, LPMSG_SERIAL, params, 0)

cdef class SessionParamBucket:
    """ params[key] to params.key

        An interface to LMDB to get and set shared session params.
    """
    def __cinit__(self, Instrument instrument):
        self.instrument = instrument

    def __getattr__(self, key):
        return self.get(key)

    def get(self, str key, object default=None):
        cdef u_int32_t param_hash
        cdef int32_t default_i32
        cdef float default_f
        v = default

        if key not in self.instrument.instrument_param_type_map:
            logger.warning('Unknown param "%s"' % key)
            return default

        param_type = self.instrument.instrument_param_type_map[key]
        param_hash = self.instrument.instrument_param_hash_map[key]

        if param_type == 'int':
            default_i32 = int(default or 0)
            v = self.instrument.get_session_int_param(param_hash, default_i32)

        elif param_type == 'float':
            default_f = float(default or 0)
            v = self.instrument.get_session_float_param(param_hash, default_f)

        else:
            logger.warning('Incompatible param type %s' % param_type)

        return v

    def __setattr__(self, str key, object value):
        cdef u_int32_t param_hash
        cdef int32_t val_i32
        cdef float val_f

        if key not in self.instrument.instrument_param_type_map:
            logger.warning('Unknown param "%s"' % key)
            return

        param_type = self.instrument.instrument_param_type_map[key]
        param_hash = self.instrument.instrument_param_hash_map[key]

        if param_type == 'int':
            val_i32 = int(value or 0)
            self.instrument.set_session_int_param(param_hash, val_i32)

        elif param_type == 'float':
            val_f = float(value or 0)
            self.instrument.set_session_float_param(param_hash, val_f)

        else:
            logger.warning('Incompatible param type %s' % param_type)

cdef class ParamBucket:
    """ params[key] to params.key

        These params are passed in to the render context 
        through the play message to the renderer.
    """
    def __init__(self, str play_params=None):
        self._play_params = play_params

    def __getattr__(self, key):
        return self.get(key)

    def get(self, key, default=None):
        if self._params is None:
            self._params = self._parse_play_params()
        return self._params.get(key, default)

    def _parse_play_params(self):
        cdef dict params = {}
        cdef dict _params
        cdef str t, k, v

        if self._play_params is not None:
            _params = {}
            for t in self._play_params.split(' '):
                if '=' in t:
                    t = t.strip()
                    k, v = tuple(t.split('='))
                    _params[k] = v
            params.update(_params)
        return params

cdef class EventContext:
    def __cinit__(self, 
            Instrument instrument,
            str msg=None,
            dict cache=None,
            object graph=None,
            int voice_id=-1,
            int default_midi_device=1,
            double max_processing_time=1,
            size_t count=0,
        ):

        cdef double now = 0

        self.graph = graph
        self.cache = cache
        self.p = ParamBucket(msg)
        self.s = SessionParamBucket(instrument) 
        self.t = EventTriggerFactory()
        self.m = MidiEventListenerProxy(default_midi_device)
        self.instrument = instrument
        self.vid = voice_id
        self.count = count
        if lpscheduler_get_now_seconds(&now) < 0:
            logger.exception('Error getting now seconds during %s event ctx init' % instrument.name)
            now = 0
        self.now = now

    def adc(self, length=1, offset=0, channels=2):
        return self.instrument.read_from_adc(length, offset=offset, channels=channels)

    def sample(self, str name, SoundBuffer snd=None):
        if snd is None:
            return self.instrument.read_from_sampler(name)
        self.instrument.save_to_sampler(name, snd)

    def resample(self, length=1, offset=0, channels=2, samplerate=48000, instrument=None):
        return self.instrument.read_from_resampler(length, offset=offset, channels=channels, samplerate=samplerate, instrument=instrument)

    def log(self, msg):
        logger.info('ctx.log[%s] %s' % (self.instrument_name, msg))

    def get_params(self):
        return self.p._params

cdef class Instrument:
    def __cinit__(self, str name, str path, int channels, double adc_length, double resampler_length, str midi_device_name):
        cdef char * midi_device_cstr
        self.midi_device_name = NULL
        if midi_device_name is not None:
            midi_device_name_byte_string = midi_device_name.encode('UTF-8')
            midi_device_cstr = midi_device_name_byte_string
            self.midi_device_name = <char *>calloc(LPMAXNAME, sizeof(char))
            strncpy(self.midi_device_name, midi_device_cstr, LPMAXNAME-1)

        instrument_byte_string = name.encode('UTF-8')
        cdef char * _instrument_ascii_name = instrument_byte_string
        # _instrument_ascii_name will get garbage collected at the end of this function
        self.ascii_name = <char *>calloc(LPMAXNAME, sizeof(char))
        strncpy(self.ascii_name, _instrument_ascii_name, LPMAXNAME-1)

        self.name = name
        self.path = path
        self.cache = {}
        self.last_reload = 0
        self.max_processing_time = 0

        self.i = astrid_instrument_start(self.ascii_name, channels, 1, adc_length, resampler_length, NULL, NULL, 
                                         self.midi_device_name,
                                         NULL, NULL, NULL, NULL)
        if self.i == NULL:
            raise InstrumentError('Could not initialize lpinstrument_t')

        # eh, why not. might be useful if the Instrument is available in the ctx too
        self.samplerate = self.i.samplerate
        self.channels = self.i.channels

        self.load_renderer(name, path)

    def __dealloc__(self):
        free(self.ascii_name)

    def load_renderer(self, name, path):
        """ Loads a renderer module from the script 
            at self.path 

            Failure to load the module raises an 
            InstrumentNotFoundError
        """
        cdef char * _k
        try:
            logger.debug('Loading instrument %s from %s' % (name, path))
            spec = importlib.util.spec_from_file_location(name, path)
            if spec is not None:
                renderer = importlib.util.module_from_spec(spec)
                try:
                    spec.loader.exec_module(renderer)
                except Exception as e:
                    logger.exception('Error loading instrument module: %s' % str(e))

                if not hasattr(renderer, '_'):
                    renderer._ = None

                self.renderer = renderer

                # fill the cache if there is something to fill:
                if hasattr(self.renderer, 'cache'):
                    self.cache = self.renderer.cache()

                if hasattr(self.renderer, 'stream'):
                    ctx = self.get_event_context()
                    self.graph = self.renderer.stream(ctx)

                if hasattr(self.renderer, 'MIDI_DEVICE'):
                    self.default_midi_device = self.renderer.MIDI_DEVICE
                else:
                    self.default_midi_device = 1

                if hasattr(self.renderer, 'PARAMS'):
                    logger.error('mapping params...')
                    self.instrument_param_type_map = self.renderer.PARAMS
                    self.instrument_param_hash_map = dict()
                    for k, t in self.instrument_param_type_map.items():
                        logger.error('mapping %s...' % k)
                        k_byte_string = k.encode('UTF-8')
                        _k = k_byte_string
                        logger.error('        %s...' % _k)
                        self.instrument_param_hash_map[k] = lphashstr(_k)
                        logger.error('        %d...' % self.instrument_param_hash_map[k])

            else:
                logger.error('Could not load instrument - spec is None: %s %s' % (path, name))
                raise InstrumentError('Instrument renderer has a null spec')

        except TypeError as e:
            logger.exception('TypeError loading renderer module: %s' % str(e))
            raise InstrumentError('Problem loading the python renderer') from e

        logger.info('loaded instrument, setting metadata')
        self.last_reload = os.path.getmtime(path)

    cpdef lpmsg_t get_message(Instrument self):
        cdef lpmsg_t msg
        if astrid_msgq_read(self.i.exmsgq, &msg) < 0:
            raise InstrumentError('Could not get the instrument message')
        self.msg = msg # so many copies omg
        return msg

    cpdef EventContext get_event_context(Instrument self, str msgstr=None, bint with_graph=False):
        cdef bytes render_params = self.msg.msg

        if msgstr is None:
            msgstr = ''
            if self.msg.type != LPMSG_MIDI_FROM_DEVICE:
                # FIXME use the is_encoded flag or something...
                # or maybe I can ignore this and just decode from utf-8 like a criminal...
                msgstr = render_params.decode('ascii')

        logger.error('PLAY PARAMS: %s' % msgstr)

        graph = None
        if with_graph:
            graph = ugens.Graph()

        return EventContext.__new__(EventContext,
            self,
            msg=msgstr,
            cache=self.cache,
            graph=graph,
            voice_id=self.msg.voice_id,
            max_processing_time=self.max_processing_time,
            default_midi_device=self.default_midi_device,
            count=self.msg.count,
        )

    cdef SoundBuffer read_from_adc(Instrument self, double length, double offset=0, int channels=2, int samplerate=48000):
        cdef size_t i
        cdef int c
        cdef lpbuffer_t * out

        cdef SoundBuffer snd = SoundBuffer(length=length, channels=channels, samplerate=samplerate)
        cdef size_t length_in_frames = len(snd)
        cdef size_t offset_in_frames = <size_t>(offset * samplerate)

        out = LPBuffer.create(length_in_frames, channels, samplerate)

        # fixme add dcblock
        if lpsampler_read_ringbuffer_block(self.i.adcname, self.i.adcbuf, offset_in_frames, out) < 0:
            logger.error('pippi.renderer ADC read: failed to read %d frames at offset %d from ADC' % (length_in_frames, offset_in_frames))
            return snd

        for i in range(length_in_frames):
            for c in range(channels):
                snd.frames[i,c] = out.data[i * channels + c]

        LPBuffer.destroy(out)

        return snd

    cdef SoundBuffer read_from_resampler(Instrument self, double length, double offset=0, int channels=2, int samplerate=48000, str instrument=None):
        cdef size_t i
        cdef int c
        cdef lpbuffer_t * out
        cdef char * resampler_name

        cdef SoundBuffer snd = SoundBuffer(length=length, channels=channels, samplerate=samplerate)
        cdef size_t length_in_frames = len(snd)
        cdef size_t offset_in_frames = <size_t>(offset * samplerate)

        out = LPBuffer.create(length_in_frames, channels, samplerate)

        if instrument is None:
            resampler_name = self.i.resamplername
        else:
            instrument = '%s-resampler' % instrument
            resampler_name_bytes = instrument.encode('UTF-8')
            resampler_name = resampler_name_bytes

        if lpsampler_read_ringbuffer_block(resampler_name, self.i.resamplerbuf, offset_in_frames, out) < 0:
            logger.error('pippi.renderer resampler read: failed to read %d frames at offset %d from resampler' % (length_in_frames, offset_in_frames))
            return snd

        for i in range(length_in_frames):
            for c in range(channels):
                snd.frames[i,c] = lpfilternan(out.data[i * channels + c])

        LPBuffer.destroy(out)

        return snd

    cdef SoundBuffer read_from_sampler(Instrument self, str name):
        cdef size_t i
        cdef int c
        cdef lpbuffer_t * out
        cdef SoundBuffer snd 

        name_bytes = name.encode('UTF-8')
        cdef char * _name = name_bytes

        out = lpsampler_aquire_and_map(_name)

        if out == NULL or out.length <= 0 or out.channels <= 0:
            logger.error('pippi.renderer sampler read: failed to read from %s' % _name)
            if out != NULL:
                logger.error('length=%s channels=%s' % (out.length, out.channels))
            return None

        snd = SoundBuffer(framelength=out.length, channels=self.channels, samplerate=self.samplerate)
        logger.error('out.length=%s len(snd)=%s self.samplerate=%s' % (out.length, len(snd), self.samplerate))

        if len(snd) == 0:
            logger.error('BLOWUP! self.channels=%d self.samplerate=%s' % (self.channels, self.samplerate))
            return None

        for i in range(out.length):
            for c in range(out.channels):
                snd.frames[i,c] = out.data[i * out.channels + c]

        if lpsampler_release_and_unmap(_name, out) < 0:
            logger.error('Could not release %s sampler memory' % _name)

        return snd

    cdef void save_to_sampler(Instrument self, str name, SoundBuffer snd):
        cdef size_t i
        cdef int c
        cdef lpbuffer_t * out

        name_bytes = name.encode('UTF-8')
        cdef char * _name = name_bytes

        # create the shm buffer
        logger.error('Creating %s buffer with frame 10,0=%s' % (name, snd.frames[10,0]))
        out = lpsampler_create(_name, snd.dur, snd.channels, <int>self.samplerate)
        logger.error('out.length=%s len(snd)=%s' % (out.length, len(snd)))

        # write the data into it
        for i in range(out.length):
            for c in range(out.channels):
                out.data[i * out.channels + c] = snd.frames[i,c]

        if lpsampler_release_and_unmap(_name, out) < 0:
            logger.error('Could not release %s sampler memory' % _name)


    def reload(self):
        logger.debug('Reloading instrument %s from %s' % (self.name, self.path))
        spec = importlib.util.spec_from_file_location(self.name, self.path)
        if spec is not None:
            renderer = importlib.util.module_from_spec(spec)
            try:
                spec.loader.exec_module(renderer)
            except Exception as e:
                logger.exception('Error reloading instrument module: %s' % str(e))

            if not hasattr(renderer, '_'):
                renderer._ = None

            self.renderer = renderer
        else:
            logger.error('Error reloading instrument. Null spec at path:\n  %s' % self.path)

        # fill the cache again if there is something to fill:
        if hasattr(self.renderer, 'cache'):
            self.cache = self.renderer.cache()

        if hasattr(self.renderer, 'stream'):
            ctx = self.get_event_context()
            self.graph = self.renderer.stream(ctx)


    def get_session_int_param(self, u_int32_t hash_key, int default):
        logger.debug('Getting INT param from key %d (with default %d)' % (hash_key, default))
        return astrid_instrument_get_param_int32(self.i, hash_key, default)

    def get_session_float_param(self, u_int32_t hash_key, float default):
        return astrid_instrument_get_param_float(self.i, hash_key, default)

    def set_session_int_param(self, u_int32_t hash_key, int value):
        logger.debug('Setting INT param to key %d' % hash_key)
        astrid_instrument_set_param_int32(self.i, hash_key, value)

    def set_session_float_param(self, u_int32_t hash_key, float value):
        astrid_instrument_set_param_float(self.i, hash_key, value)

    def handle_update_message(self, str msgstr):
        cdef EventContext ctx 
        cdef dict params
        cdef str p, k, v
        cdef size_t last_edit

        last_edit = os.path.getmtime(self.path)
        if last_edit > self.last_reload:
            self.reload()
            self.last_reload = last_edit

        if not hasattr(self.renderer, 'update'):
            logger.warning('Ignoring update message: this instrument has no callback registered')
            return None

        ctx = self.get_event_context()
        params = dict()

        try:
            # read msg body, split and call update with each
            for p in msgstr.split(' '):
                if '=' in p:
                    p = p.strip()
                    k, v = tuple(p.split('='))
                else:
                    v = None
                    k = p.strip()
                    if k == '':
                        continue
                self.renderer.update(ctx, k.strip(), v.strip())
        except Exception as e:
            logger.exception('Error during %s update message handling: %s' % (self.name, e))

    def handle_midi_message(self, char * payload):
        cdef EventContext ctx 
        cdef size_t offset = 0
        cdef unsigned char mtype=0, mid=0, mval=0
        if not hasattr(self.renderer, 'midi_messages'):
            logger.warning('Ignoring MIDI message: this instrument has no callback registered')
            return None

        memcpy(&mtype, payload, sizeof(unsigned char))
        offset += sizeof(unsigned char)
        memcpy(&mid, payload+offset, sizeof(unsigned char))
        offset += sizeof(unsigned char)
        memcpy(&mval, payload+offset, sizeof(unsigned char))

        logger.debug(f'PY handle_midi_message: {mtype=} {mid=} {mval=}')
        ctx = self.get_event_context()
        self.renderer.midi_messages(ctx, mtype, mid, mval)

cdef tuple collect_players(Instrument instrument):
    loop = False
    # FIXME it's still nice to support this, but 
    # it should just schedule a message instead of 
    # storing a flag on the serialized buffer...
    if hasattr(instrument.renderer, 'LOOP'):
        loop = instrument.renderer.LOOP

    # find all play functions
    players = set()

    # The simplest case is a single play function
    if hasattr(instrument.renderer, 'play'):
        players.add(instrument.renderer.play)

    # Play functions can also be registered via 
    # an @player.init decorator
    if hasattr(instrument.renderer, 'player') \
        and hasattr(instrument.renderer.player, 'players') \
        and isinstance(instrument.renderer.player.players, set):
        players |= instrument.renderer.player.players

    # Finally, any compatible function can be added to a set
    if hasattr(instrument.renderer, 'PLAYERS') \
        and isinstance(instrument.renderer.PLAYERS, set):
        players |= instrument.renderer.PLAYERS
    
    return players, loop

cdef int render_event(Instrument instrument, str msgstr):
    cdef set players
    cdef bint loop
    cdef bytes bufstr
    cdef bytes render_params = instrument.msg.msg
    cdef int dacid = 0
    cdef EventContext ctx 
    instrument_byte_string = instrument.name.encode('UTF-8')
    cdef char * _instrument_ascii_name = instrument_byte_string

    ctx = instrument.get_event_context(msgstr)

    logger.debug('rendering event %s w/params %s' % (str(instrument), msgstr))

    if hasattr(instrument.renderer, 'before'):
        instrument.renderer.before(ctx)

    players, loop = collect_players(instrument)

    for player in players:
        try:
            ctx.count = 0
            ctx.tick = 0
            generator = player(ctx)

            try:
                for snd in generator:
                    if snd is None:
                        continue

                    if isinstance(snd, tuple):
                        dacid, snd = snd
                    else:
                        dacid = 0

                    bufstr = serialize_buffer(snd, loop, instrument.msg)
                    astrid_instrument_publish_bufstr(_instrument_ascii_name, bufstr, len(bufstr))

            except Exception as e:
                logger.exception('Error during %s generator render: %s' % (instrument.name, e))
                return 1
        except Exception as e:
            logger.exception('Error allocating generator for %s render: %s' % (instrument.name, e))
            return 1

    if hasattr(instrument.renderer, 'done'):
        instrument.renderer.done(ctx)

    return 0

cdef set collect_trigger_planners(Instrument instrument):
    # find all trigger planner functions
    planners = set()

    # The simplest case is a single trigger function
    if hasattr(instrument.renderer, 'trigger'):
        planners.add(instrument.renderer.trigger)

    # Trigger functions can also be registered via 
    # a @triggerer.init decorator
    if hasattr(instrument.renderer, 'triggerer') \
        and hasattr(instrument.renderer.triggerer, 'planners') \
        and isinstance(instrument.renderer.triggerer.planners, set):
        planners |= instrument.renderer.triggerer.planners

    # Finally, any compatible function can be added to a set
    if hasattr(instrument.renderer, 'TRIGGERERS') \
        and isinstance(instrument.renderer.TRIGGERERS, set):
        planners |= instrument.renderer.TRIGGERERS
    
    return planners

cdef int trigger_events(Instrument instrument):
    """ Collect the trigger functions in the instrument module
        and compute the triggers to be scheduled.
    """
    cdef set planners
    cdef bint loop
    cdef EventContext ctx 
    cdef list eventlist
    cdef int qfd
    cdef double now = 0
    cdef bytes trigger_params = instrument.msg.msg
    cdef list trigger_events = []

    ctx = instrument.get_event_context()

    #logger.debug('trigger generation event %s w/params %s' % (str(instrument), trigger_params))

    if hasattr(instrument.renderer, 'trigger_before'):
        instrument.renderer.trigger_before(ctx)

    # Collect the planners and collate the triggers
    planners = collect_trigger_planners(instrument)

    for p in planners:
        ctx.count = 0
        ctx.tick = 0
        try:
            eventlist = p(ctx)
            if eventlist is None:
                continue
            trigger_events += eventlist
        except Exception as e:
            logger.exception('Error during %s trigger generation: %s' % (ctx.instrument.name, e))
            return 1

    # Schedule the trigger events
    #logger.debug('Scheduling %d trigger events: %s' % (len(trigger_events), trigger_events))

    if lpscheduler_get_now_seconds(&now) < 0:
        logger.exception('Error getting now seconds during %s trigger scheduling' % ctx.instrument_name)
        now = 0

    for t in trigger_events:
        if t is None:
            logger.debug('Got null trigger in event list')
            continue
        if t.schedule(now) < 0:
            logger.exception('Error trying to schedule event from %s trigger generation' % ctx.instrument_name)
        #logger.debug('Scheduled event %s' % t)

    if hasattr(instrument.renderer, 'trigger_done'):
        instrument.renderer.trigger_done(ctx)

    return 0

def render_executor(Instrument instrument, object q, int comrade_id):
    cdef size_t last_edit
    cdef double start=0, end=0

    # Prevent multiple processes from sharing the same seed
    dsp.seed(time.time() + comrade_id)

    while instrument.i.is_running:
        msg = q.get()

        if msg is None:
            logger.info('Renderer comrade %d shutting down' % comrade_id)
            break

        last_edit = os.path.getmtime(instrument.path)
        if last_edit > instrument.last_reload:
            instrument.reload()
            instrument.last_reload = last_edit

        if lpscheduler_get_now_seconds(&start) < 0:
            logger.exception('Error getting now seconds')
            return

        if render_event(instrument, msg) < 0:
            logger.exception('Error trying to execute python render...')
            return

        if lpscheduler_get_now_seconds(&end) < 0:
            logger.exception('Error getting now seconds')
            return

        instrument.max_processing_time = max(instrument.max_processing_time, end - start)
        logger.info('%s render time: %f seconds' % (instrument.name, end - start))

cdef int astrid_schedule_python_triggers(Instrument instrument) except -1:
    cdef size_t last_edit = os.path.getmtime(instrument.path)

    # Reload instrument
    if last_edit > instrument.last_reload:
        instrument.reload()
        instrument.last_reload = last_edit

    try:
        return trigger_events(instrument)
    except Exception as e:
        logger.exception('Error during scheduling of python triggers: %s' % e)
        return -1

def _run_forever(Instrument instrument, 
        str script_path, 
        str instrument_name, 
        int channels, 
        double adc_length,
        double resampler_length,
        object q, 
    ):
    cdef lpmsg_t msg

    logger.info(f'PY: running forever... {script_path=} {instrument_name=}')

    while True:
        logger.info('PY MSG: waiting for a message...')
        try:
            msg = instrument.get_message()
        except InstrumentError as e:
            print('There was a problem reading from the msg q. Maybe try turning it off and on again?')
            continue

        logger.info('PY MSG: got one!')

        if msg.type == LPMSG_SHUTDOWN:
            logger.info('PY MSG: shutdown')
            for _ in range(NUM_COMRADES):
                q.put(None)
            break

        elif msg.type == LPMSG_UPDATE:
            logger.info('PY MSG: update')
            instrument.handle_update_message(msg.msg.decode('utf-8'))

        elif msg.type == LPMSG_MIDI_FROM_DEVICE:
            logger.info('PY MSG: midi from device')
            instrument.handle_midi_message(msg.msg)

        elif msg.type == LPMSG_MIDI_TO_DEVICE:
            logger.info('PY MSG: midi to device')

        elif msg.type == LPMSG_PLAY:
            logger.info('PY MSG: play')
            logger.error('PY MSG: play params: %s' % msg.msg)
            q.put(msg.msg.decode('utf-8'))

        elif msg.type == LPMSG_TRIGGER:
            logger.info('PY MSG: trigger')
            if astrid_schedule_python_triggers(instrument) < 0:
                logger.error('Error trying to schedule python triggers...')

        elif msg.type == LPMSG_LOAD:
            # FIXME pass this along to the comrades
            try:
                if instrument is None:
                    instrument = Instrument(instrument_name, script_path, channels, adc_length, resampler_length)
                else:
                    last_edit = os.path.getmtime(instrument.path)
                    if last_edit > instrument.last_reload:
                        instrument.reload()
                        instrument.last_reload = last_edit
            except InstrumentError as e:
                logger.error('PY: Error trying to reload instrument. Shutting down...')
                break

    logger.info('PY: python instrument shutting down...')

def run_forever(str script_path, str instrument_name=None, int channels=2, double adc_length=30, double resampler_length=30, str midi_device_name=None):
    cdef Instrument instrument = None
    instrument_name = instrument_name if instrument_name is not None else Path(script_path).stem
    instrument_byte_string = instrument_name.encode('UTF-8')
    cdef char * _instrument_ascii_name = instrument_byte_string

    try:
        # Start the stream and setup the instrument
        logger.info(f'PY: loading python instrument... {script_path=} {instrument_name=} {midi_device_name=}')
        instrument = Instrument(instrument_name, script_path, channels, adc_length, resampler_length, midi_device_name)
        logger.info(f'PY: started instrument... {script_path=} {instrument_name=}')
    except InstrumentError as e:
        logger.error('PY: Error trying to start instrument. Shutting down...')
        return

    render_pool = []
    render_q = SimpleQueue()
    for i in range(NUM_COMRADES):
        comrade = Process(target=render_executor, args=(instrument, render_q, i))
        comrade.start()
        render_pool += [ comrade ]

    message_process = Process(target=_run_forever, args=(instrument, script_path, instrument_name, channels, adc_length, resampler_length, render_q))
    message_process.start()

    try:
        while instrument.i.is_running:
            # Read messages from the console and relay them to the q
            # times out after a second to allow for render pool cleanup
            if astrid_instrument_tick(instrument.i) < 0:
                logger.info('PY: Could not read console line')
                time.sleep(2)
                continue

            for comrade in render_pool:
                if not comrade.is_alive():
                    render_pool.remove(comrade)
                    logger.error('Our comrade has become exhausted, and is being relieved')
                    comrade = Process(target=render_executor, args=(instrument, render_q, i))
                    comrade.start()
                    render_pool += [ comrade ]

    except KeyboardInterrupt as e:
        print('PY: Got keyboard interrupt')

    print('Shutting down...')
    message_process.join()
    for r in render_pool:
        r.join()
    print('All done!')
