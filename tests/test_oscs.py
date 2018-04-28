import random
from unittest import TestCase

from pippi.oscs import Osc
from pippi.soundbuffer import SoundBuffer
from pippi import dsp

class TestOscs(TestCase):
    def test_create_sinewave(self):
        osc = Osc(dsp.SINE, freq=random.triangular(20, 20000))
        length = random.triangular(0.01, 1)
        out = osc.play(length)
        self.assertEqual(len(out), int(length * out.samplerate))

        wtA = [ random.random() for _ in range(random.randint(10, 1000)) ]
        osc = Osc(wtA, freq=random.triangular(20, 20000))
        length = random.triangular(0.01, 1)
        out = osc.play(length)
        self.assertEqual(len(out), int(length * out.samplerate))

        wtB = dsp.wt([ random.random() for _ in range(random.randint(10, 1000)) ])
        osc = Osc(wtB, freq=random.triangular(20, 20000))
        length = random.triangular(0.01, 1)
        out = osc.play(length)
        self.assertEqual(len(out), int(length * out.samplerate))

        wtC = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        osc = Osc(wtC, freq=random.triangular(20, 20000))
        length = random.triangular(0.01, 1)
        out = osc.play(length)
        self.assertEqual(len(out), int(length * out.samplerate))

    def test_create_wt_stack(self):
        wtA = [ random.random() for _ in range(random.randint(10, 1000)) ]
        wtB = dsp.wt([ random.random() for _ in range(random.randint(10, 1000)) ])
        wtC = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        stack = [dsp.RND, wtA, wtB, wtC] * 10
        osc = Osc(stack=stack, freq=random.triangular(20, 20000))
        length = random.triangular(0.01, 1)
        out = osc.play(length)

        self.assertEqual(len(out), int(length * out.samplerate))


