import dsp
import __main__
import multiprocessing as mp
import pygame.midi
import pyaudio
import time
import os
import alsaaudio

class MidiManager():
    def __init__(self, ns):
        self.ns = ns
        self.offset = None

    def setOffset(self, offset):
        self.offset = offset

    def geti(self, cc, default=None, low=0, high=1):
        return int(round(self.get(cc, default, low, high)))

    def getr(self, cc, default=None, low=0, high=1, spread=1):
        spread = 1 if spread > 1 else spread
        spread = dsp.rand(0, spread)

        value = self.get(cc, default, low, high)

        return value * spread

    def getri(self, cc, default=None, low=0, high=1, spread=1):
        return int(round(self.getr(cc, default, low, high, spread)))

    def get(self, cc, default=None, low=0, high=1):
        if self.offset is not None:
            cc = cc + self.offset

        try:
            value = getattr(self.ns, 'cc%s' % cc)
            return (int(value) / 127.0) * (high - low) + low
        except Exception:
            if default is None:
                default = low

            return default

class ParamManager():
    def __init__(self, ns):
        self.ns = ns
        self.namespace = 'global'

    def setNamespace(self, namespace):
        self.namespace = namespace

    def set(self, param, value, namespace=None):
        if namespace is None:
            namespace = self.namespace

        params = self.getAll(namespace)

        params[param] = value

        setattr(self.ns, namespace, params)

    def get(self, param, default=None, namespace=None):
        if namespace is None:
            namespace = self.namespace

        params = self.getAll(namespace)

        # TODO param type conversion, yay...
        return params.get(param, default)

    def getAll(self, namespace=None):
        if namespace is None:
            namespace = self.namespace

        try:
            params = getattr(self.ns, namespace)
        except AttributeError:
            params = {}

        return params

class IOManager():
    def __init__(self):
        self.manager = mp.Manager()
        self.ns = self.manager.Namespace()

        self.ns.device = 'T6'

        self.m = mp.Process(target=self.capture_midi, args=(self.ns,))
        self.m.start()

    def __del__(self):
        self.out.stop_stream()
        self.out.close()
        self.p.terminate()
        del self.md

    def capture_midi(self, ns):
        try:
            pygame.midi.init()
            self.md = pygame.midi.Input(3) # TODO add to config / init

            while True:
                if self.md.poll():
                    midi_events = self.md.read(10)

                    for e in midi_events:
                        # timestamp = e[1]
                        # cc = e[0][1]
                        # value = e[0][2]
                        setattr(ns, 'cc%s' % e[0][1], e[0][2])

                time.sleep(0.05)

        except pygame.midi.MidiException:
            dsp.log('Midi device not found')

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

    def play(self, gen, ns):
        gen = __import__(gen)
        midi_manager = MidiManager(ns)
        param_manager = ParamManager(ns)

        out = self.open_alsa_pcm(ns.device)

        def dsp_loop(out, play, midi_manager, param_manager):
            os.nice(-2)
            snd = play(midi_manager, param_manager)
            snd = dsp.split(snd, 500)
            for s in snd:
                out.write(s)

        while True:
            playback = mp.Process(target=dsp_loop, args=(out, gen.play, midi_manager, param_manager))
            playback.start()
            playback.join()

            reload(gen)

        return snd
