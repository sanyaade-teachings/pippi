#cython: language_level=3

import array
from cpython cimport array
from libc.stdlib cimport calloc
from libc.string cimport memcpy
import logging
from logging.handlers import SysLogHandler
import warnings
import importlib
import importlib.util
import os
from pathlib import Path
import struct
import threading

import redis

from pippi import dsp
from pippi.soundbuffer cimport SoundBuffer

ADC_NAME = 'adc'

logger = logging.getLogger('astrid-renderer')
if not logger.handlers:
    logger.addHandler(SysLogHandler(address='/dev/log', facility=SysLogHandler.LOG_DAEMON))
    logger.setLevel(logging.DEBUG)
    warnings.simplefilter('always')

_redis = redis.StrictRedis(host='localhost', port=6379, db=0)
bus = _redis.pubsub()

cdef bytes serialize_buffer(SoundBuffer buf, size_t onset, int is_looping, str name, str play_params):
    cdef bytearray strbuf
    cdef size_t audiosize, length, namesize, paramsize
    cdef int channels, samplerate

    channels = <int>buf.channels
    samplerate = <int>buf.samplerate
    length = <size_t>len(buf)

    # size of audio data 
    audiosize = length * channels * sizeof(lpfloat_t)
    namesize = <size_t>len(name)

    paramsize = 0
    if play_params is not None:
        paramsize = <size_t>len(play_params)

    strbuf = bytearray()

    strbuf += struct.pack('N', audiosize)
    strbuf += struct.pack('N', namesize)
    strbuf += struct.pack('N', paramsize)
    strbuf += struct.pack('N', length)
    strbuf += struct.pack('i', channels)
    strbuf += struct.pack('i', samplerate)
    strbuf += struct.pack('i', is_looping)
    strbuf += struct.pack('N', onset)

    for i in range(length):
        for c in range(channels):
            strbuf += struct.pack('d', buf.frames[i,c])
    strbuf += bytes(name, 'ascii')

    if play_params is not None:
        strbuf += bytes(play_params, 'ascii')

    return bytes(strbuf)

cdef SoundBuffer read_from_adc(double length, double offset=0, int channels=2, int samplerate=48000):
    cdef size_t i
    cdef size_t pos = 0
    cdef int c
    cdef lpadcbuf_t * adcbuf = lpadc_open()

    cdef SoundBuffer snd = SoundBuffer(length=length, channels=channels, samplerate=samplerate)
    cdef size_t framelength = len(snd)
    lpadc_get_pos(adcbuf, &pos)

    for i in range(framelength):
        for c in range(channels):
            snd.frames[i,c] = lpadc_read_sample(adcbuf, pos-i, c)

    lpadc_close(adcbuf)

    return snd

cdef void create_from_sampler(SoundBuffer snd, int bankid):
    pass

cdef SoundBuffer dub_into_sampler(SoundBuffer snd, int bankid):
    return snd

cdef SoundBuffer read_from_sampler(int bankid):
    cdef size_t i
    cdef size_t pos = 0
    cdef int c
    cdef lpsampler_t * sampler = lpsampler_open(bankid)

    cdef size_t framelength = 0
    cdef int samplerate = 0
    cdef int channels = 0

    lpsampler_get_length(sampler, &framelength)
    lpsampler_get_samplerate(sampler, &samplerate)
    lpsampler_get_channels(sampler, &channels)

    cdef SoundBuffer snd = SoundBuffer(length=framelength/<float>samplerate, channels=channels, samplerate=samplerate)

    for i in range(framelength):
        for c in range(channels):
            snd.frames[i,c] = lpsampler_read_sample(sampler, i, c)

    lpsampler_close(sampler)

    return snd

cdef class SessionParamBucket:
    """ params[key] to params.key
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
    """
    def __init__(self, dict instrument_params, str play_params=None):
        self._instrument_params = instrument_params
        self._play_params = play_params

    def __getattr__(self, key):
        return self.get(key)

    def get(self, key, default=None):
        if self._params is None:
            self._params = self._parse_play_params()
        return self._params.get(key, default)

    def _parse_play_params(self):
        cdef dict params = self._instrument_params or {}
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
    def __init__(self, 
            instrument_params=None, 
            instrument_name=None, 
            play_params=None,
            sounds=None,
            midi_devices=None, 
            midi_maps=None, 
            dict cache=None
        ):

        self.cache = cache
        self.p = ParamBucket(instrument_params, play_params)
        self.s = SessionParamBucket() 
        self.client = None
        self.instrument_name = instrument_name
        self.sounds = sounds
        self.play_params = play_params
        #self.sampler = Sampler()

    def play(self, instrument_name, *params, **kwargs):
        """ FIXME -- this should probably be a general 
            message-passing interface, so instrument scripts
            can invoke any astrid orchestration commands.
        """
        if params is not None:
            params = params[0]

        if params is None:
            params = {}

        if kwargs is not None:
            params.update(kwargs)

        try:
            rcmd = 'INSTRUMENT_PATH="orc/%s.py" INSTRUMENT_NAME="%s" ./build/renderer' % (instrument_name, instrument_name)
            subprocess.Popen(rcmd, shell=True)
        except Exception as e:
            logger.exception('Could not start renderer from within renderer: %s' % e)

    def adc(self, length=1, offset=0, channels=2):
        return read_from_adc(length, offset=offset, channels=channels)

    def sampler(self, bankid=0):
        bankid %= 10000
        return read_from_sampler(bankid)

    def write_sampler(self, buf, bankid):
        bankid %= 10000
        create_from_sampler(buf, bankid)

    def dub_sampler(self, buf, bankid):
        bankid %= 10000
        return dub_into_sampler(buf, bankid)

    def log(self, msg):
        logger.info(msg)

    def get_params(self):
        return self.p._params

cdef class Instrument:
    def __init__(self, str name, str path, object renderer):
        self.name = name
        self.path = path
        self.renderer = renderer
        self.sounds = self.load_sounds()
        self.playing = <int>1
        self.params = {}
        self.cache = {}

        if hasattr(renderer, 'groups') and (\
                isinstance(renderer.groups, list) or \
                isinstance(renderer.groups, tuple)):
            self.groups = list(renderer.groups)
        else:
            self.groups = []

    def reload(self):
        logger.debug('Reloading instrument %s from %s' % (self.name, self.path))
        spec = importlib.util.spec_from_file_location(self.name, self.path)
        if spec is not None:
            renderer = importlib.util.module_from_spec(spec)
            try:
                spec.loader.exec_module(renderer)
            except Exception as e:
                logger.exception('Error loading instrument module: %s' % str(e))

            self.renderer = renderer
        else:
            logger.error(self.path)

    def load_sounds(self):
        if hasattr(self.renderer, 'SOUNDS') and isinstance(self.renderer.SOUNDS, list):
            return [ dsp.read(snd) for snd in self.renderer.SOUNDS ]
        elif hasattr(self.renderer, 'SOUNDS') and isinstance(self.renderer.SOUNDS, dict):
            return { k: dsp.read(snd) for k, snd in self.renderer.SOUNDS.items() }
        return None

    def map_midi_devices(self):
        device_aliases = []
        midi_maps = {}

        if hasattr(self.renderer, 'MIDI'): 
            if isinstance(self.renderer.MIDI, list):
                device_aliases = self.renderer.MIDI
            else:
                device_aliases = [ self.renderer.MIDI ]

        for i, device in enumerate(device_aliases):
            mapping = None
            if hasattr(self.renderer, 'MAP'):
                if isinstance(self.renderer.MAP, list):
                    try:
                        mapping = self.renderer.MAP[i]
                    except IndexError:
                        pass
                else:
                    mapping = self.renderer.MAP

                midi_maps[device] = mapping 

        return device_aliases, midi_maps

    def create_ctx(self, dict instrument_params, str play_params):
        device_aliases, midi_maps = self.map_midi_devices()
        return EventContext(
                    instrument_params=instrument_params, 
                    instrument_name=self.name, 
                    play_params=play_params,
                    sounds=self.sounds,
                    midi_devices=device_aliases, 
                    midi_maps=midi_maps, 
                    cache=self.cache,
                )



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
        #logger.info('Loading instrument %s from %s' % (name, path))
        spec = importlib.util.spec_from_file_location(name, path)
        if spec is not None:
            renderer = importlib.util.module_from_spec(spec)
            try:
                spec.loader.exec_module(renderer)
            except Exception as e:
                logger.exception('Error loading instrument module: %s' % str(e))

            instrument = Instrument(name, path, renderer)

        else:
            logger.error('Could not load instrument - spec is None: %s %s' % (path, name))
            raise InstrumentNotFoundError(name)
    except TypeError as e:
        logger.exception('TypeError loading instrument module: %s' % str(e))
        raise InstrumentNotFoundError(name) from e

    #midi.start_listener(instrument)
    return instrument

def default_onsets(ctx):
    yield 0

cdef tuple collect_players(object instrument):
    loop = False
    if hasattr(instrument.renderer, 'LOOP'):
        loop = instrument.renderer.LOOP

    overlap = 1
    if hasattr(instrument.renderer, 'OVERLAP'):
        overlap = instrument.renderer.OVERLAP

    # find all play methods
    players = set()

    # The simplest case is a single play method 
    # with an optional onset list or callback
    if hasattr(instrument.renderer, 'play'):
        onsets = getattr(instrument.renderer, 'onsets', default_onsets)
        players.add((instrument.renderer.play, onsets))

    # Play methods can also be registered via 
    # an @player.init decorator, which also registers 
    # an optional onset list or callback
    if hasattr(instrument.renderer, 'player') \
        and hasattr(instrument.renderer.player, 'players') \
        and isinstance(instrument.renderer.player.players, set):
        players |= instrument.renderer.player.players

    if hasattr(instrument.renderer, 'PLAYERS') \
        and isinstance(instrument.renderer.PLAYERS, set):
        players |= instrument.renderer.PLAYERS
    
    return players, loop, overlap

cdef int render_event(object instrument, str params):
    cdef set players
    cdef object onset_generator
    cdef bint loop
    cdef double overlap
    cdef EventContext ctx = instrument.create_ctx(instrument.params, params)

    logger.debug('rendering event %s w/params %s' % (str(instrument), params))

    if hasattr(instrument.renderer, 'before'):
        instrument.renderer.before(ctx)

    players, loop, overlap = collect_players(instrument)

    for player, onsets in players:
        try:
            ctx.count = 0
            ctx.tick = 0

            # FIXME use the onset generator with play_sequence, dummy!
            if onsets is None:
                onset_generator = default_onsets(ctx)
            else:
                onset_generator = onsets(ctx)
            #play_sequence(buf_q, player, ctx, onset_generator, loop, overlap)

            generator = player(ctx)
            try:
                for snd in generator:
                    bufstr = serialize_buffer(snd, 0, loop, ctx.instrument_name, ctx.play_params)
                    _redis.publish('astridbuffers', bufstr)
            except Exception as e:
                logger.exception('Error during %s generator render: %s' % (ctx.instrument_name, e))
                return 1
        except Exception as e:
            logger.exception('Error during %s play sequence: %s' % (ctx.instrument_name, e))
            return 1

    if hasattr(instrument.renderer, 'done'):
        # When the loop has completed or playback has stopped, 
        # execute the done callback
        instrument.renderer.done(ctx)

    return 0

ASTRID_INSTRUMENT = None

cdef public int astrid_load_instrument() except -1:
    global ASTRID_INSTRUMENT
    path = os.environ.get('INSTRUMENT_PATH', 'orc/ding.py')
    name = Path(path).stem
    os.environ['INSTRUMENT_PATH'] = path
    os.environ['INSTRUMENT_NAME'] = name

    # finally, actually load the damn instrument module
    ASTRID_INSTRUMENT = _load_instrument(name, path)

    # subscribe to all our favorite redis channels
    bus.subscribe('astrid-message', 'astrid-message-%s' % name)
    for group in ASTRID_INSTRUMENT.groups:
        bus.subscribe('astrid-group-message-%s' % group)

    return 0

cdef public int astrid_tick(char msg[LPMAXMSG], size_t * length, size_t * timestamp) except -1:
    global ASTRID_INSTRUMENT

    cdef Py_ssize_t _length = length[0]
    cdef size_t _timestamp = timestamp[0]

    # Reload instrument
    ASTRID_INSTRUMENT.reload()
    return render_event(ASTRID_INSTRUMENT, msg[:_length].decode('UTF-8'))



