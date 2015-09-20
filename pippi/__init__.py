import dsp
import __main__
import multiprocessing as mp
import time
import os
import sys

# Try to enable midi support
try:
    import pygame.midi
except ImportError:
    print 'Midi support disabled. Please install pygame.'


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



class EventManager():
    def __init__(self, ns):
        self.ns = ns

    def grid(self, tick, bpm):
        os.nice(0)

        beat = dsp.bpm2frames(bpm)

        count = 0
        while getattr(self.ns, 'grid', True):
            tick.set()
            tick.clear()
            dsp.delay(beat)
            count += 1

class MidiManager():
    def __init__(self, device_id, ns, device_type=None):
        self.device_id = device_id
        self.ns = ns
        self.device_type = 'input' if device_type is None else device_type
        self.offset = None

        if self.device_type == 'output':
            pygame.midi.init()
            self.output = pygame.midi.Output(int(self.device_id))

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
            value = getattr(self.ns, 'cc%s%s' % (self.device_id, cc))
            return (int(value) / 127.0) * (high - low) + low
        except Exception:
            if default is None:
                default = high 

            return default

    def noteon(self, value):
        self.output.note_on(value)

    def noteoff(self, value):
        self.output.note_off(value)

class ParamManager():
    def __init__(self, ns):
        self.ns = ns
        self.namespace = 'global'

    def setNamespace(self, namespace):
        self.namespace = namespace

    def set(self, param, value, namespace=None, throttle=None):
        if namespace is None:
            namespace = self.namespace

        if throttle is not None:
            last_updated = self.get('%s-last_updated' % param, time.time(), namespace='meta')

            if time.time() - last_updated >= throttle:
                self.set('%s-last_updated' % param, time.time(), namespace='meta')
                self.set(param, value, namespace)

        else:
            params = self.getAll(namespace)
            params[param] = value
            setattr(self.ns, namespace, params)

    def get(self, param, default=None, namespace=None, throttle=None):
        if namespace is None:
            namespace = self.namespace

        if throttle is not None:
            last_updated = self.get('%s-last_updated' % param, time.time(), namespace='meta')

            if time.time() - last_updated >= throttle:
                self.set('%s-last_updated' % param, time.time(), namespace='meta')
                self.set(param, default, namespace)

            return self.get(name, default, namespace)

        params = self.getAll(namespace)

        value = params.get(param, None)

        if value is None:
            value = default
            self.set(param, value, namespace)

        return value

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

        self.ns.device = 'default'

    def __del__(self):
        self.out.stop_stream()
        self.out.close()
        self.p.terminate()

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

    def play(self, generator, ns, voice_id, voice_index, loop=True):
        sys.path.insert(0, os.getcwd())
        gen = __import__(generator)

        midi_devices = {}

        if hasattr(gen, 'midi'):
            for device, device_id in gen.midi.iteritems():
                dsp.log('\ndevice: %s device_id: %s' % (device, device_id))

                pygame.midi.init()
                device_info = pygame.midi.get_device_info(device_id)
                device_type = 'input' if device_info[2] else 'output'
                pygame.midi.quit()

                try:
                    midi_devices[device] = MidiManager(device_id, ns, device_type)
                    dsp.log('setting midi manager %s' % device_id)
                except:
                    dsp.log('Could not load midi device %s with id %s' % (device, device_id))

        param_manager = ParamManager(ns)

        if audio_engine == 'alsa':
            out = self.open_alsa_pcm(ns.device)
        elif audio_engine == 'portaudio':
            out = self.open_pyaudio_pcm(ns.device)
        else:
            print 'Playback is disabled.'
            return False

        try:
            os.nice(-2)
        except OSError:
            os.nice(0)

        group = None
        if hasattr(gen, 'groups'):
            group = gen.groups[ voice_index % len(gen.groups) ]

        if hasattr(gen, 'sbank'):
            sbank = {}
            for snd in gen.sbank:
                sbank[snd[0]] = dsp.read(snd[1]).data
        else:
            sbank = None

        meta = {
            'midi': midi_devices,
            'param': param_manager,
            'id': voice_id,
            'group': group, 
            'sbank': sbank
        }

        if not hasattr(ns, 'reload'):
            setattr(ns, 'reload', False)

        iterations = 0
        while getattr(ns, '%s-%s-loop' % (generator, voice_id)) == True:
            meta['iterations'] = iterations
            iterations += 1

            if getattr(ns, 'reload') == True and not hasattr(gen, 'automate'):
                reload(gen)

            # automate will always override play
            if hasattr(gen, 'play') and not hasattr(gen, 'automate'):
                snd = gen.play(meta)
                snd = dsp.split(snd, 500)
                for s in snd:
                    try:
                        out.write(s)
                    except AttributeError:
                        dsp.log('Could not write to audio device')
                        return False

            if hasattr(gen, 'automate'):
                gen.automate(meta)

                if hasattr(gen, 'loop_time'):
                    time.sleep(gen.loop_time)

        return True
