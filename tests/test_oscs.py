import random
from unittest import TestCase

from pippi.oscs import Osc, Osc2d, Pulsar
from pippi.soundbuffer import SoundBuffer
from pippi import dsp

class TestOscs(TestCase):
    def test_create_sinewave(self):
        osc = Osc(dsp.SINE, freq=200.0)
        length = 1
        out = osc.play(length)
        out.write('tests/renders/osc_sinewave.wav')
        self.assertEqual(len(out), int(length * out.samplerate))

        wtA = [ random.random() for _ in range(random.randint(10, 1000)) ]
        osc = Osc(wtA, freq=200.0)
        length = 1
        out = osc.play(length)
        out.write('tests/renders/osc_rand_list_wt.wav')
        self.assertEqual(len(out), int(length * out.samplerate))

        wtB = dsp.wt([ random.random() for _ in range(random.randint(10, 1000)) ])
        osc = Osc(wtB, freq=200.0)
        length = 1
        out = osc.play(length)
        out.write('tests/renders/osc_rand_wt_wt.wav')
        self.assertEqual(len(out), int(length * out.samplerate))

        wtC = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        osc = Osc(wtC, freq=200.0)
        length = 1
        out = osc.play(length)
        out.write('tests/renders/osc_guitar_wt.wav')
        self.assertEqual(len(out), int(length * out.samplerate))

    def test_create_wt_stack(self):
        wtA = [ random.random() for _ in range(random.randint(10, 1000)) ]
        wtB = dsp.wt([ random.random() for _ in range(random.randint(10, 1000)) ])
        wtC = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        stack = [dsp.RND, wtA, wtB, wtC] * 10
        osc = Osc2d(stack, freq=200.0)
        length = 1
        out = osc.play(length)
        out.write('tests/renders/osc2d_RND_randlist_randwt_guitar_10x.wav')

        self.assertEqual(len(out), int(length * out.samplerate))

    def test_create_pulsar(self):
        osc = Pulsar(
                dsp.SINE, 
                window=dsp.SINE, 
                pulsewidth=dsp.wt(dsp.TRI, 0, 1), 
                freq=200.0, 
                amp=0.2
            )
        length = 10
        out = osc.play(length)
        out.write('tests/renders/osc_pulsar.wav')
        self.assertEqual(len(out), int(length * out.samplerate))



