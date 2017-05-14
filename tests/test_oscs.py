import random
from unittest import TestCase

from pippi.oscs import Osc

class TestOscs(TestCase):
    def test_create_sinewave(self):
        osc = Osc(random.triangular(20, 20000), wavetable='sine')
        length = random.randint(1, 44100)
        out = osc.play(length)

        self.assertEqual(len(out), length)


