#cython: language_level=3

import array
from cpython cimport array
from libc.stdlib cimport calloc, free
from libc.string cimport strcpy, memcpy
import logging
from logging.handlers import SysLogHandler
import warnings
import importlib
import importlib.util
import os
from pathlib import Path
import platform
import struct
import subprocess
import threading

import redis

from pippi import dsp, midi
from pippi.soundbuffer cimport SoundBuffer


logger = logging.getLogger('astrid-cyrenderer')
if not logger.handlers:
    if platform.system() == 'Darwin':
        log_path = '/var/run/syslog'
    else:
        log_path = '/dev/log'

    logger.addHandler(SysLogHandler(address=log_path))
    logger.setLevel(logging.DEBUG)
    warnings.simplefilter('always')

_redis = redis.StrictRedis(host='localhost', port=6379, db=0)
bus = _redis.pubsub()

cdef lpfloat_t[LPADCBUFSAMPLES] adc_block

cdef bytes serialize_buffer(SoundBuffer buf, int is_looping, lpmsg_t * msg):
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
            strbuf += struct.pack('d', buf.frames[i,c])

    msgview = <unsigned char[:msgsize]>(<unsigned char *>msg)
    for i in range(msgsize):
        strbuf.append(msgview[i])

    return bytes(strbuf)

cdef SoundBuffer read_from_adc(double length, double offset=0, int channels=2, int samplerate=48000):
    cdef size_t i
    cdef int c

    cdef SoundBuffer snd = SoundBuffer(length=length, channels=channels, samplerate=samplerate)
    cdef size_t length_in_frames = len(snd)
    cdef size_t offset_in_frames = <size_t>(offset * samplerate)

    if lpadc_read_block_of_samples(offset_in_frames * channels, length_in_frames * channels, adc_block) < 0:
        logger.error('cyrenderer ADC read: failed to read %d frames at offset %d from ADC' % (length_in_frames, offset_in_frames))
        return snd

    for i in range(length_in_frames):
        for c in range(channels):
            snd.frames[i,c] = adc_block[i * channels + c]

    return snd


cdef class MessageEvent:
    def __cinit__(self,
            double onset,
            str instrument_name,
            int msgtype,
            str params,
            double max_processing_time
        ):

        cdef size_t onset_frames = 0

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

        strcpy(self.msg.msg, byte_params)
        strcpy(self.msg.instrument_name, byte_instrument_name)

    cpdef int schedule(MessageEvent self, double now):
        self.msg.initiated = now
        #logger.debug('CYRENDERER: Sending message type %d to %s' % (self.msg.type, self.msg.instrument_name))
        #logger.debug('initiated=%f scheduled=%f voice_id=%d' % (self.msg.initiated, self.msg.scheduled, self.msg.voice_id))
        return send_message(self.msg[0])

    def __dealloc__(self):
        if self.msg is not NULL:
            free(self.msg)

cdef class SerialEvent:
    def __cinit__(self,
            double onset,
            lpmsg_t msg,
            int group=0, 
            int device=0
        ):

        self.event = <lpserialevent_t *>calloc(1, sizeof(lpserialevent_t))
        self.event.onset = onset
        self.event.now = 0
        self.event.msg = msg
        self.event.group = group
        self.event.device = device

    cpdef int schedule(SerialEvent self, double now):
        """
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
        """

        return 0 

    def __dealloc__(self):
        if self.event is not NULL:
            free(self.event)


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
        global ASTRID_INSTRUMENT
        params = self._parse_params(*args, **kwargs)
        return MessageEvent(onset, instrument_name, LPMSG_PLAY, params, ASTRID_INSTRUMENT.max_processing_time)

    def trigger(self, double onset, str instrument_name, *args, **kwargs):
        global ASTRID_INSTRUMENT
        params = self._parse_params(*args, **kwargs)
        return MessageEvent(onset, instrument_name, LPMSG_TRIGGER, params, ASTRID_INSTRUMENT.max_processing_time)



cdef class SessionParamBucket:
    """ params[key] to params.key

        An interface to redis to get and set shared session params.
    """
    def __getattr__(self, key):
        return self.get(key)

    def get(self, key, default=None):
        v = _redis.get(key.encode('ascii')) 
        if v is None:
            return default
        return v.decode('utf-8')

    def __setattr__(self, key, value):
        _redis.set(key.encode('ascii'), value.encode('ascii'))

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
            str instrument_name=None, 
            str msg=None,
            dict cache=None,
            int voice_id=-1,
            int default_midi_device=1,
            double max_processing_time=1,
            size_t count=0,
        ):

        cdef double now = 0

        self.cache = cache
        self.p = ParamBucket(str(msg))
        self.s = SessionParamBucket() 
        self.t = EventTriggerFactory()
        self.m = MidiEventListenerProxy(default_midi_device)
        self.instrument_name = instrument_name
        self.vid = voice_id
        self.count = count
        if lpscheduler_get_now_seconds(&now) < 0:
            logger.exception('Error getting now seconds during %s event ctx init' % instrument_name)
            now = 0
        self.now = now

    def adc(self, length=1, offset=0, channels=2):
        return read_from_adc(length, offset=offset, channels=channels)

    def log(self, msg):
        logger.info('ctx.log[%s] %s' % (self.instrument_name, msg))

    def get_params(self):
        return self.p._params

cdef class Instrument:
    def __init__(self, str name, str path, object renderer):
        self.name = name
        self.path = path
        self.renderer = renderer
        self.cache = {}
        self.last_reload = 0
        self.max_processing_time = 0

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
            self.register_midi_triggers()
        else:
            logger.error('Error reloading instrument. Null spec at path:\n  %s' % self.path)

        # fill the cache again if there is something to fill:
        if hasattr(self.renderer, 'cache'):
            self.cache = self.renderer.cache()

    def register_midi_triggers(self):
        if hasattr(self.renderer, 'MIDI'): 
            devices = []
            if isinstance(self.renderer.MIDI, list):
                devices += self.renderer.MIDI
            else:
                devices = [ self.renderer.MIDI ]

            try:
                for device, low, high in devices:
                    _redis.hset('%s-triggers' % device, self.name, 1)
                    _redis.set('%s-%s-triglow' % (device, self.name), str(low))
                    _redis.set('%s-%s-trighigh' % (device, self.name), str(high))
            except (TypeError, ValueError):
                logger.error('Could not unpack MIDI params from %s: %s' % (self.name, devices))


class InstrumentNotFoundError(Exception):
    def __init__(self, instrument_name, *args, **kwargs):
        self.message = 'No instrument named %s found' % instrument_name

def _load_instrument(name, path):
    """ Loads a renderer module from the script 
        at self.path 

        Failure to load the module raises an 
        InstrumentNotFoundError
    """
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

            instrument = Instrument(name, path, renderer)

            # fill the cache if there is something to fill:
            if hasattr(instrument.renderer, 'cache'):
                instrument.cache = instrument.renderer.cache()

            if hasattr(instrument.renderer, 'MIDI_DEVICE'):
                instrument.default_midi_device = instrument.renderer.MIDI_DEVICE
            else:
                instrument.default_midi_device = 1

        else:
            logger.error('Could not load instrument - spec is None: %s %s' % (path, name))
            raise InstrumentNotFoundError(name)
    except TypeError as e:
        logger.exception('TypeError loading instrument module: %s' % str(e))
        raise InstrumentNotFoundError(name) from e

    instrument.last_reload = os.path.getmtime(path)
    instrument.register_midi_triggers()
    return instrument

cdef tuple collect_players(object instrument):
    loop = False
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

cdef int render_event(object instrument, lpmsg_t * msg):
    cdef set players
    cdef bint loop
    cdef EventContext ctx 
    cdef str msgstr
    cdef bytes bufstr
    cdef bytes render_params = msg.msg
    cdef int dacid = 0

    msgstr = render_params.decode('ascii')
    ctx = EventContext.__new__(EventContext,
        instrument_name=instrument.name, 
        msg=msgstr,
        cache=instrument.cache,
        voice_id=msg.voice_id,
        default_midi_device=instrument.default_midi_device,
        max_processing_time=instrument.max_processing_time,
        count=msg.count,
    )

    #logger.debug('rendering event %s w/params %s' % (str(instrument), render_params))

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

                    bufstr = serialize_buffer(snd, loop, msg)
                    _redis.publish('astrid-dac-%d-bufferfeed' % dacid, bufstr)

            except Exception as e:
                logger.exception('Error during %s generator render: %s' % (ctx.instrument_name, e))
                return 1
        except Exception as e:
            logger.exception('Error allocating generator for %s render: %s' % (ctx.instrument_name, e))
            return 1

    if hasattr(instrument.renderer, 'done'):
        instrument.renderer.done(ctx)

    return 0

cdef set collect_trigger_planners(object instrument):
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

cdef int trigger_events(object instrument, lpmsg_t * msg):
    """ Collect the trigger functions in the instrument module
        and compute the triggers to be scheduled.
    """
    cdef set planners
    cdef bint loop
    cdef EventContext ctx 
    cdef str msgstr
    cdef list eventlist
    cdef int qfd
    cdef double now = 0
    cdef bytes trigger_params = msg.msg
    cdef list trigger_events = []

    msgstr = trigger_params.decode('ascii')
    ctx = EventContext.__new__(EventContext,
        instrument_name=instrument.name, 
        msg=msgstr,
        cache=instrument.cache,
        voice_id=msg.voice_id,
        max_processing_time=instrument.max_processing_time,
        default_midi_device=instrument.default_midi_device,
        count=msg.count,
    )

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
            logger.exception('Error during %s trigger generation: %s' % (ctx.instrument_name, e))
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


ASTRID_INSTRUMENT = None

cdef public int astrid_reload_instrument(char * path) except -1:
    cdef size_t last_edit 
    global ASTRID_INSTRUMENT

    if ASTRID_INSTRUMENT is None:
        _path = path.decode('utf-8')
        name = Path(_path).stem
        ASTRID_INSTRUMENT = _load_instrument(name, _path)
        return 0

    last_edit = os.path.getmtime(ASTRID_INSTRUMENT.path)
    if last_edit > ASTRID_INSTRUMENT.last_reload:
        ASTRID_INSTRUMENT.reload()
        ASTRID_INSTRUMENT.last_reload = last_edit

    return 0


cdef public int astrid_load_instrument(char * path) except -1:
    global ASTRID_INSTRUMENT
    _path = path.decode('utf-8')
    name = Path(_path).stem

    ASTRID_INSTRUMENT = _load_instrument(name, _path)
    return 0

cdef public int astrid_schedule_python_render(void * msgp) except -1:
    global ASTRID_INSTRUMENT
    cdef lpmsg_t * msg = <lpmsg_t *>msgp
    cdef size_t last_edit = os.path.getmtime(ASTRID_INSTRUMENT.path)
    cdef int render_result
    cdef double start = 0
    cdef double end = 0

    if lpscheduler_get_now_seconds(&start) < 0:
        logger.exception('Error getting now seconds')
        return 1

    # Reload instrument
    if last_edit > ASTRID_INSTRUMENT.last_reload:
        ASTRID_INSTRUMENT.reload()
        ASTRID_INSTRUMENT.last_reload = last_edit

    render_result = render_event(ASTRID_INSTRUMENT, msg)

    if lpscheduler_get_now_seconds(&end) < 0:
        logger.exception('Error getting now seconds')
        return 1

    ASTRID_INSTRUMENT.max_processing_time = max(ASTRID_INSTRUMENT.max_processing_time, end - start)
    #logger.info('%s render time: %f seconds' % (ASTRID_INSTRUMENT.name, end - start))

    return render_result


cdef public int astrid_schedule_python_triggers(void * msgp) except -1:
    global ASTRID_INSTRUMENT
    cdef lpmsg_t * msg = <lpmsg_t *>msgp
    cdef size_t last_edit = os.path.getmtime(ASTRID_INSTRUMENT.path)

    # Reload instrument
    if last_edit > ASTRID_INSTRUMENT.last_reload:
        ASTRID_INSTRUMENT.reload()
        ASTRID_INSTRUMENT.last_reload = last_edit

    try:
        return trigger_events(ASTRID_INSTRUMENT, msg)
    except Exception as e:
        logger.exception('Error during scheduling of python triggers: %s' % e)
        return -1

