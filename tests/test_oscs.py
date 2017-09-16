import random
from unittest import TestCase

from pippi.oscs import Osc

class TestOscs(TestCase):
    def test_create_sinewave(self):
        osc = Osc('sine', random.triangular(20, 20000))
        length = random.randint(1, 44100)
        out = osc.play(length)

        self.assertEqual(len(out), length)


