import dsp
import glob
import os
import sys
import time
import __main__
import multiprocessing as mp
import midi
from params import ParamManager

# Doing a fallback dance for playback support
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
        self.manager    = mp.Manager()
        self.ns         = self.manager.Namespace()

        self.ns.device  = 'default'
        self.ns.grid    = False
        self.tick       = mp.Event()

        self.generators = self.findGenerators()
        self.looping    = {}
        self.armed      = []

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
        return None

    def loadGenerator(self, generator_name):
        sys.path.insert(0, os.getcwd())
        return __import__(generator_name)

    def armGenerator(self, generator_name):
        generator_name = self.validateGenerator(generator_name)

        armed = getattr(self.ns, 'armed', [])

        if generator_name is not None and generator_name not in armed:
            dsp.log('arming %s...' % generator_name)
            armed += [ generator_name ]
            setattr(self.ns, 'armed', armed)

            gen = self.loadGenerator(generator_name)
            ctl = self.setupCtl(gen)

            trigger = midi.MidiTrigger(gen.trigger['device'], gen.trigger['notes'])

            respawn = mp.Event()

            p = mp.Process(target=self._armGenerator, args=(generator_name, gen, ctl, trigger, respawn))
            p.start()

            while generator_name in armed:
                respawn.wait()
                respawn.clear()
                dsp.log('rearming %s...' % generator_name)
                p = mp.Process(target=self._armGenerator, args=(generator_name, gen, ctl, trigger, respawn))
                p.start()
                armed = getattr(self.ns, 'armed', [])


    def playGenerator(self, generator_name, num_voices=1, loop=True):
        generator_name = self.validateGenerator(generator_name)

        if generator_name is not None:
            if generator_name not in self.looping:
                self.looping[generator_name] = {}

            gen = self.loadGenerator(generator_name)
            ctl = self.setupCtl(gen)

            for _ in range(num_voices):
                voice_id = len(self.looping[generator_name]) + 1

                setattr(self.ns, '%s-%s-loop' % (generator_name, voice_id), loop)
                p = mp.Process(target=self._playGenerator, args=(generator_name, gen, voice_id, ctl))
                p.start()

                self.looping[generator_name][voice_id] = p

    def _armGenerator(self, generator_name, gen, ctl, trigger, respawn):
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
            self.tick.wait()

        out.write(snd)

        del out


    def _playGenerator(self, generator_name, gen, voice_id, ctl):
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
                self.tick.wait()

            out.write(snd)

            if getattr(self.ns, '%s-%s-loop' % (generator_name, voice_id)) == False:
                break

        delattr(self.ns, 'buffer-%s-%s' % (generator_name, voice_id))

    def setupCtl(self, gen):
        param_manager = ParamManager(self.ns)

        midi_readers = None
        if hasattr(gen, 'midi'):
            mappings = None
            if hasattr(gen, 'mappings'):
                mappings = gen.mappings

            midi_readers = midi.get_midi_readers(gen.midi, mappings, self.ns)

        group = None
        if hasattr(gen, 'groups'):
            group = gen.groups[ voice_index % len(gen.groups) ]

        if hasattr(gen, 'samples'):
            samples = {}
            for snd in gen.samples:
                samples[snd[0]] = dsp.read(snd[1]).data
        else:
            samples = None

        ctl = {
            'param': param_manager,
            'midi': midi_readers,
            'group': group, 
            'samples': samples,
            'note': None # for armed voices, this will be populated with note/velocity info
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
        self.grid = mp.Process(name='grid', target=self.gridHandler, args=(self.tick, bpm))
        self.grid.start()
        self.ns.grid = True

    def gridHandler(self, tick, bpm, midiout=4):
        # TODO: send MIDI clock again, assign device via 
        # config file and/or console cmd. Also allow mapping 
        # MIDI control to grid tempo value without resetting
        os.nice(0)

        cdiv = 24

        beat = dsp.bpm2frames(bpm) / cdiv

        count = 0

        ts = time.time()

        while getattr(self.ns, 'grid', True):
            if count % cdiv == 0:
                tick.set()
                tick.clear()

            ts = time.time()

            dsp.delay(beat)
            count += 1

        ts = time.time()

    def set_bpm(self, bpm):
        self.grid.terminate()
        self.start_grid(bpm)
 
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

