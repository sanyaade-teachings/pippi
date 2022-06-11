#cython: language_level=3

import array
from cpython cimport array
import logging
from logging.handlers import SysLogHandler
import warnings
import importlib
import importlib.util
import os
from pathlib import Path
import threading

import numpy as np
#cimport numpy as np
import redis

#np.import_array()

from pippi import dsp
from pippi.soundbuffer cimport SoundBuffer

ADC_NAME = 'adc'

logger = logging.getLogger('pippi-renderer')
if not logger.handlers:
    logger.addHandler(SysLogHandler(address='/dev/log', facility=SysLogHandler.LOG_DAEMON))
    logger.addHandler(logging.StreamHandler())
    logger.setLevel(logging.DEBUG)
    warnings.simplefilter('always')

_redis = redis.StrictRedis(host='localhost', port=6379, db=0)
bus = _redis.pubsub()

cdef SoundBuffer read_from_adc(double length, tuple channels=None, double offset=0, int samplerate=48000):
    if channels is None:
        channels = (0,1)

    cdef array.array a
    cdef int framelength = <int>(length * samplerate)
    cdef int o = <int>(offset * samplerate)

    bytelist = b''.join(_redis.lrange(ADC_NAME, o, o+framelength-1))
    a = array.array('d', bytelist)

    return SoundBuffer(np.ndarray(shape=(framelength, len(channels)), buffer=a, dtype='d'), channels=len(channels), samplerate=samplerate)

cdef class SessionParamBucket:
    """ params[key] to params.key
    """
    def __init__(self):
        self._bus = {}

    def __getattr__(self, key):
        return self.get(key)

    def get(self, key, default=None):
        v = self._bus.get(key) 
        if v is None:
            return default
        return v.decode('utf-8')

cdef class ParamBucket:
    """ params[key] to params.key
    """
    def __init__(self, params):
        self._params = params

    def __getattr__(self, key):
        return self.get(key)

    def get(self, key, default=None):
        if self._params is None:
            return default
        return self._params.get(key, default)

cdef class EventContext:
    def __init__(self, 
            params=None, 
            instrument_name=None, 
            sounds=None,
            midi_devices=None, 
            midi_maps=None, 
            before=None,
            messages=None,
        ):

        self.before = before
        self.messages = messages or []
        self.p = ParamBucket(params)
        self.s = SessionParamBucket() 
        self.client = None
        self.instrument_name = instrument_name
        self.sounds = sounds
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

        #if self.client is not None:
        #    self.client.send_cmd([names.PLAY_INSTRUMENT, instrument_name, params])

    def adc(self, length=1):
        return read_from_adc(length)

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
        self.messages = {}
        self.playing = <int>1
        self.params = {}

        if hasattr(renderer, 'groups') and (\
                isinstance(renderer.groups, list) or \
                isinstance(renderer.groups, tuple)):
            self.groups = list(renderer.groups)
        else:
            self.groups = []

    def reload(self):
        #logger.info('Reloading instrument %s from %s' % (self.name, self.path))
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

    def flush_messages(self):
        for k in self.messages.keys():
            self.messages[k] = []

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

    def create_ctx(self, params, messages):
        device_aliases, midi_maps = self.map_midi_devices()
        return EventContext(
                    params=params, 
                    messages=messages,
                    instrument_name=self.name, 
                    sounds=self.sounds,
                    midi_devices=device_aliases, 
                    midi_maps=midi_maps, 
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
    
    #logger.info('COLLECT_PLAYERS players: %s' % players)
    return players, loop, overlap

cdef list render_event(object instrument, object params, object buf_q):
    cdef set players
    cdef object onset_generator
    cdef bint loop
    cdef double overlap
    cdef EventContext ctx = instrument.create_ctx(instrument.params, instrument.messages)
    cdef list out = []

    #logger.debug('getting players')
    players, loop, overlap = collect_players(instrument)
    #logger.debug(str(players))

    for player, onsets in players:
        try:
            ctx.count = 0
            ctx.tick = 0

            if onsets is None:
                onset_generator = default_onsets(ctx)
            else:
                onset_generator = onsets(ctx)
            
            #play_sequence(buf_q, player, ctx, onset_generator, loop, overlap)
            generator = player(ctx)
            try:
                for snd in generator:
                    out += [ snd ]
            except Exception as e:
                logger.exception('Error during %s generator render: %s' % (ctx.instrument_name, e))
        except Exception as e:
            logger.exception('Error during %s play sequence: %s' % (ctx.instrument_name, e))

    if hasattr(instrument.renderer, 'done'):
        # When the loop has completed or playback has stopped, 
        # execute the done callback
        instrument.renderer.done(ctx)

    #print('Rendered %s buffers for event %s' % (len(out), instrument))
    return out


ASTRID_ADC_BUFFERS = []
ASTRID_RENDERS = []
ASTRID_INSTRUMENT = None

cdef public int astrid_load_instrument() except -1:
    global ASTRID_INSTRUMENT
    path = os.environ.get('INSTRUMENT_PATH', 'orc/ding.py')
    name = Path(path).stem
    ASTRID_INSTRUMENT = _load_instrument(name, path)
    #logger.debug("Loaded %s (%s)" % (name, path))
    bus.subscribe('astrid', name)
    for group in ASTRID_INSTRUMENT.groups:
        bus.subscribe('group%s' % group)
    return 0

cdef public int astrid_reload_instrument() except -1:
    global ASTRID_INSTRUMENT
    ASTRID_INSTRUMENT.reload()
    #logger.debug("Reloaded %s" % ASTRID_INSTRUMENT)
    return 0

cdef public int astrid_render_event() except -1:
    global ASTRID_RENDERS
    global ASTRID_INSTRUMENT

    ASTRID_RENDERS += render_event(ASTRID_INSTRUMENT, None, None)
    return 0

cdef public int astrid_get_messages() except -1:
    global ASTRID_RENDERS
    global ASTRID_INSTRUMENT

    message = bus.get_message()
    if message is not None:
        print('MESSAGE', message)
        if message['type'] == 'message':
            msg = message['data'].decode('utf-8')
            channel = message['channel'].decode('utf-8')
            if channel not in ASTRID_INSTRUMENT.messages:
                ASTRID_INSTRUMENT.messages[channel] = []
            ASTRID_INSTRUMENT.messages[channel] += [ msg ]
    return 0

cdef public int astrid_get_instrument_status(int * status) except -1:
    global ASTRID_INSTRUMENT

    for channel, messages in ASTRID_INSTRUMENT.messages.items():
        for msg in messages:
            if msg == 'stop':
                ASTRID_INSTRUMENT.playing = <int>0
            elif msg == 'play':
                ASTRID_INSTRUMENT.playing = <int>1
                ASTRID_INSTRUMENT.params['group'] = channel
            elif msg.startswith('setval'):
                _, key, coding, val = msg.split(':')
                if coding == 'int':
                    c = int
                elif coding == 'float':
                    c = float
                else:
                    c = str

                ASTRID_INSTRUMENT.params[key] = c(val)
                print(ASTRID_INSTRUMENT.params)

    ASTRID_INSTRUMENT.flush_messages()

    status[0] = ASTRID_INSTRUMENT.playing
    return 0

cdef public int astrid_get_info(size_t * length, int * channels, int * samplerate) except -1:
    global ASTRID_RENDERS

    length[0] = <size_t>len(ASTRID_RENDERS[-1])
    channels[0] = <int>(ASTRID_RENDERS[-1].channels)
    samplerate[0] = <int>(ASTRID_RENDERS[-1].samplerate)

    return 0

cdef public int astrid_buffer_count(int * count) except -1:
    global ASTRID_RENDERS
    count[0] = <int>len(ASTRID_RENDERS)

cdef public int astrid_copy_buffer(lpbuffer_t * buffer) except -1:
    cdef size_t i
    cdef int c
    global ASTRID_RENDERS

    snd = ASTRID_RENDERS.pop()
    for i in range(buffer.length):
        for c in range(buffer.channels):
            buffer.data[i * buffer.channels + c] = snd.frames[i][c]


