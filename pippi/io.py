import dsp
import glob
import os
import sys
import time
import __main__
import multiprocessing as mp
import midi
import mido
from params import ParamManager
import config
import seq
import tune
import osc

try:
    import alsaaudio
    audio_engine = 'alsa'
except ImportError:
    try:
        import pyaudio
        audio_engine = 'portaudio'
        print 'Using experimental portaudio bindings. Please install alsaaudio if you have problems with sound playback.'
    except ImportError:
        print 'Playback disabled. Please install alsaaudio (ALSA) or pyaudio (PortAudio)'

class IOManager:
    def __init__(self):
        self.manager                = mp.Manager()
        self.ns                     = self.manager.Namespace()
        self.config                 = config.init()
        self.default_midi_device    = self.config.get('device', None)
        self.ns.device              = 'default'
        self.ns.grid                = False
        self.divcc                  = self.config.get('divcc', None)
        self.divs                   = [24, 12, 6, 8, 3] # Defines the order for MIDI control

        self.ticks = {
            24: mp.Event(), # Quarter note
            12: mp.Event(), # Eighth note
            8: mp.Event(),  # Eighth note triplets
            6: mp.Event(),  # Sixteenth note
            3: mp.Event(),  # 32nd note
        }

        self.generators             = self.findGenerators()
        self.looping                = {}
        self.armed                  = []
        self.patterns               = {}
        self.osc_servers            = {}

    def startOscServer(self, port=None):
        if port not in self.osc_servers:
            self.osc_servers[port] = osc.OscListener(port, self.ns)
            self.osc_servers[port].start()

    def findGenerators(self):
        generators = []

        try:
            gens = glob.glob("*.py")
            for filename in gens:
                # Get base filename and strip .py extension
                filename = os.path.basename(filename)[:-3]
                generators += [ filename ]

            if len(generators) > 0:
                return generators

            dsp.log('Discovered %s available generator scripts' % len(generators))

        except OSError:
            pass

        dsp.log('No generators found')

        return False

    def validateGenerator(self, generator_name):
        if generator_name in self.generators:
            return generator_name
        return False

    def loadGenerator(self, generator_name):
        sys.path.insert(0, os.getcwd())
        return __import__(generator_name)

    def loadGridCtl(self):
        midi.register_midi_listener(self.default_midi_device, self.ns)
        return midi.MidiReader(self.default_midi_device, self.ns)

    def _playOneshot(self, note, velocity, length, generator_name):
        generator_name = self.validateGenerator(generator_name)

        if generator_name is not None:
            out = self.openAudioDevice()
            gen = self.loadGenerator(generator_name)
            ctl = self.setupCtl(gen)
            ctl['note'] = (note, velocity, length)
            snd = gen.play(ctl)
            out.write(snd)

    def _playPattern(self, pattern):
        def _sendMidi(note, velocity, length, device):
            velocity = int(round(velocity * 127))
            note = int(note)
            dsp.log(device)
            out = mido.open_output(device)
            msg = mido.Message('note_on', note=note, velocity=velocity)
            out.send(msg)

            dsp.delay(length)

            msg = mido.Message('note_off', note=note, velocity=0)
            out.send(msg)

        def _sendOsc():
            pass

        device = getattr(self.ns, 'selected-pattern-device', self.default_midi_device)
        devices = midi.list_devices()['output']
        dsp.log(devices)

        if self.validateGenerator(device):
            handler = self._playOneshot
        elif midi.validate_output_device_by_id(device, devices):
            device = midi.validate_output_device_by_id(device, devices) # FIXME dummy
            handler = _sendMidi
        elif osc.validateAddress(device):
            handler = _sendOsc
        else:
            # Fallback to internal generator
            handler = self._playOneshot
            device = 'default'

        setattr(self.ns, 'pattern-%s' % pattern.id, True)
        setattr(self.ns, 'pattern-play-%s' % pattern.id, True)

        grid_ctl = self.loadGridCtl()
        while getattr(self.ns, 'pattern-%s' % pattern.id):
            # wait for tick
            if self.ns.grid:
                div = getattr(self.ns, 'pattern-div-%s' % pattern.id)
                div = self.divs[div]
                self.ticks[div].wait()

            # freq, 0-1, frames
            note, velocity, length = pattern.next()
            note = tune.ftom(note)

            if velocity > 0 and getattr(self.ns, 'pattern-play-%s' % pattern.id):
                n = mp.Process(target=handler, args=(note, velocity, length, device))
                n.start()

    def startPattern(self, length, pat, notes, div, patid):
        pattern = seq.Pattern(length, pat, notes, div, patid, 0, self.ns)
        p = mp.Process(target=self._playPattern, args=(pattern,))
        p.start()

        pattern.process = p

        self.patterns[patid] = pattern

    def armGenerator(self, generator_name):
        generator_name = self.validateGenerator(generator_name)

        armed = getattr(self.ns, 'armed', [])

        if generator_name is not None and generator_name not in armed:
            dsp.log('arming %s...' % generator_name)
            armed += [ generator_name ]
            setattr(self.ns, 'armed', armed)

            gen = self.loadGenerator(generator_name)
            ctl = self.setupCtl(gen)
            grid_ctl = self.loadGridCtl()
            trigger = midi.MidiTrigger(gen.trigger['device'], gen.trigger['notes'])

            respawn = mp.Event()

            p = mp.Process(target=self._armGenerator, args=(generator_name, gen, ctl, trigger, respawn, grid_ctl))
            p.start()

            while generator_name in armed:
                respawn.wait()
                respawn.clear()
                p = mp.Process(target=self._armGenerator, args=(generator_name, gen, ctl, trigger, respawn, grid_ctl))
                p.start()
                armed = getattr(self.ns, 'armed', [])


    def playGenerator(self, generator_name, num_voices=1, loop=True):
        generator_name = self.validateGenerator(generator_name)

        
        if generator_name is not None:
            grid_ctl = self.loadGridCtl()

            if generator_name not in self.looping:
                self.looping[generator_name] = {}

            gen = self.loadGenerator(generator_name)
            ctl = self.setupCtl(gen)

            for _ in range(num_voices):
                voice_id = len(self.looping[generator_name]) + 1

                setattr(self.ns, '%s-%s-loop' % (generator_name, voice_id), loop)
                p = mp.Process(target=self._playGenerator, args=(generator_name, gen, voice_id, ctl, grid_ctl))
                p.start()

                self.looping[generator_name][voice_id] = p

    def _armGenerator(self, generator_name, gen, ctl, trigger, respawn, grid_ctl):
        try:
            os.nice(-2)
        except OSError:
            os.nice(0)

        voice_id = os.getpid()
        out = self.openAudioDevice()
        ctl['count'] = 1

        if getattr(self.ns, 'reload', False) == True:
            reload(gen)

        note_info = trigger.wait()
        ctl['note'] = note_info

        respawn.set()

        snd = gen.play(ctl)

        if self.ns.grid:
            div = grid_ctl.geti(self.divcc, low=0, high=4, default=0)
            div = self.divs[div]
            self.ticks[div].wait()

        out.write(snd)

        del out


    def _playGenerator(self, generator_name, gen, voice_id, ctl, grid_ctl):
        try:
            os.nice(-2)
        except OSError:
            os.nice(0)

        if not hasattr(self.ns, 'reload'):
            setattr(self.ns, 'reload', False)

        render_process = None
        def render_again(gen, ctl, generator_name, voice_id, ns):
            snd = gen.play(ctl)
            setattr(ns, 'buffer-%s-%s' % (generator_name, voice_id), snd)

        out = self.openAudioDevice()

        group = None
        if hasattr(gen, 'groups'):
            group = gen.groups[ voice_id % len(gen.groups) ]

        ctl['group'] = group

        iterations = 0
        while True:
            ctl['count'] = iterations
            iterations += 1

            if getattr(self.ns, 'reload') == True:
                reload(gen)

            if hasattr(self.ns, 'buffer-%s-%s' % (generator_name, voice_id)):
                snd = getattr(self.ns, 'buffer-%s-%s' % (generator_name, voice_id))
            else:
                # First play render
                snd = gen.play(ctl)

            if render_process is None or not render_process.is_alive():
                # async start render of next buffer
                render_process = mp.Process(name='render-%s-%s' % (generator_name, voice_id), target=render_again, args=(gen, ctl, generator_name, voice_id, self.ns))
                render_process.start()

            if self.ns.grid:
                div = grid_ctl.geti(self.divcc, low=0, high=4, default=0)
                div = self.divs[div]
                self.ticks[div].wait()

            out.write(snd)

            if getattr(self.ns, '%s-%s-loop' % (generator_name, voice_id)) == False:
                break

        try:
            delattr(self.ns, 'buffer-%s-%s' % (generator_name, voice_id))
        except AttributeError:
            dsp.log('Could not remove buffer-%s-%s' % (generator_name, voice_id))

    def setupCtl(self, gen, voice_index=0):
        param_manager = ParamManager(self.ns)
        osc_manager   = osc.OscManager(self.ns)

        midi_readers = None
        if hasattr(gen, 'midi'):
            mappings = None
            if hasattr(gen, 'mappings'):
                mappings = gen.mappings

            midi_readers = midi.get_midi_readers(gen.midi, mappings, self.ns)

        if hasattr(gen, 'samples'):
            samples = {}
            for snd in gen.samples:
                samples[snd[0]] = dsp.read(snd[1]).data
        else:
            samples = None

        ctl = {
            'param': param_manager,
            'osc': osc_manager,
            'midi': midi_readers,
            'samples': samples,
            'note': (None, None) # for armed voices, this will be populated with note/velocity info
        }

        return ctl

    def setReload(self, do_reload):
        setattr(self.ns, 'reload', do_reload)

    def openAudioDevice(self, device=None):
        if device is None:
            device = self.ns.device

        if audio_engine == 'alsa':
            out = self.open_alsa_pcm(device)
        elif audio_engine == 'portaudio':
            out = self.open_pyaudio_pcm(device)
        else:
            print 'Playback is disabled.'
            return False

        return out

    def stopAllGenerators(self, kill=False):
        for generator_name in self.looping.keys():
            self.stopGenerator(generator_name, kill)

    def stopGenerator(self, generator_name, kill=False):
        if generator_name in self.looping:
            for voice_id, process in self.looping[generator_name].iteritems():
                setattr(self.ns, '%s-%s-loop' % (generator_name, voice_id), False)

                if kill:
                    process.terminate()

            del self.looping[generator_name]
            return True
        
        dsp.log('Could not stop %s generators: none currently playing' % generator_name)
        return False

    def getArmed(self):
        return getattr(self.ns, 'armed', [])

    def disarmAllGenerators(self):
        setattr(self.ns, 'armed', [])

    def disarmGenerator(self, generator_name):
        armed = getattr(self.ns, 'armed', [])
        armed.remove(generator_name)
        setattr(self.ns, 'armed', armed)

    def stopGrid(self):
        self.ns.grid = False
        if self.grid and self.grid.is_alive():
            self.grid.terminate()

    def startGrid(self, bpm=120):
        ctl_device = self.default_midi_device
        midi.register_midi_listener(ctl_device, self.ns)
        ctl = midi.MidiReader(ctl_device, self.ns)

        self.grid = mp.Process(name='grid', target=self.gridHandler, args=(self.ticks, bpm, ctl))
        self.grid.start()
        self.ns.grid = True

    def gridHandler(self, divs, bpm, ctl=None):
        # TODO: send MIDI clock again, assign device via 
        # config file and/or console cmd. Also allow mapping 
        # MIDI control to grid tempo value without resetting
        os.nice(0)

        count = 0

        """
        divs = [
            24, # Quarter note
            12, # Eighth note
            8,  # Eighth note triplets
            6,  # Sixteenth note
            3,  # 32nd note
        ]
        """

        out = mido.open_output(self.default_midi_device)
        start = mido.Message('start')
        stop = mido.Message('stop')
        clock = mido.Message('clock')

        out.send(start)

        while getattr(self.ns, 'grid', True):
            beat = dsp.bpm2frames(bpm) / 24

            out.send(clock)

            if count % 24 == 0:
                divs[24].set()
                divs[24].clear()

            if count % 12 == 0:
                divs[12].set()
                divs[12].clear()

            if count % 8 == 0:
                divs[8].set()
                divs[8].clear()

            if count % 6 == 0:
                divs[6].set()
                divs[6].clear()

            if count % 3 == 0:
                divs[3].set()
                divs[3].clear()

            dsp.delay(beat)
            count += 1

        out.send(stop)

    def gridHandlerSeq(self, bpm, ctl=None):
        # TODO: send MIDI clock again, assign device via 
        # config file and/or console cmd. Also allow mapping 
        # MIDI control to grid tempo value without resetting
        os.nice(0)

        count = 0

        divs = [1, 2, 4, 3, 8, 16]

        out = mido.open_output('MidiSport 2x2 MIDI 1')
        clock = mido.Message('clock')

        while getattr(self.ns, 'grid', True):
            divi = ctl.geti(5, low=0, high=len(divs)-1, default=0)
            div = divs[divi] # divisions of the beat

            beat = dsp.bpm2frames(bpm) / div

            out.send(clock)

            dsp.delay(beat)
            count += 1

    def set_bpm(self, bpm):
        self.grid.terminate()
        self.startGrid(bpm)
 
    def open_alsa_pcm(self, device='default'):
        try:
            out = alsaaudio.PCM(alsaaudio.PCM_PLAYBACK, alsaaudio.PCM_NORMAL, device)
        except:
            print 'Could not open an ALSA connection.'
            return False

        out.setchannels(2)
        out.setrate(44100)
        out.setformat(alsaaudio.PCM_FORMAT_S16_LE)
        out.setperiodsize(10)

        return out

    def open_pyaudio_pcm(self, device='default'):
        self.p = pyaudio.PyAudio()

        out = self.p.open(
            format = self.p.get_format_from_width(2),
            channels = 2,
            rate = 44100,
            output = True
        )

        return out
