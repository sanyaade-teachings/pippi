import random
from unittest import TestCase

from pippi.oscs import Osc
from pippi import dsp

class TestOscs(TestCase):
    def test_create_sinewave(self):
        osc = Osc(dsp.SINE, random.triangular(20, 20000))
        length = random.triangular(0.01, 1)
        out = osc.play(length)

        self.assertEqual(len(out), int(length * out.samplerate))


