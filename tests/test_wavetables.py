import tempfile
import random

from unittest import TestCase
from pippi import wavetables

class TestWavetables(TestCase):
    def test_random_window(self):
        length = random.randint(1, 1000)
        win = wavetables.window('random', length)
        self.assertEqual(length, len(win))

    def test_bad_window_type(self):
        length = random.randint(1, 1000)
        win = wavetables.window('this is not a window type', length)
        self.assertEqual(length, len(win))

    def test_random_wavetable(self):
        length = random.randint(1, 1000)
        wt = wavetables.wavetable('random', length)
        self.assertEqual(length, len(wt))

    def test_bad_wavetable_type(self):
        length = random.randint(1, 1000)
        wt = wavetables.wavetable('this is not a wavetable type', length)
        self.assertEqual(length, len(wt))

