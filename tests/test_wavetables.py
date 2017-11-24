import tempfile
import random

from unittest import TestCase
from pippi import wavetables, dsp

class TestWavetables(TestCase):
    def test_random_window(self):
        length = random.randint(1, 1000)
        win = wavetables.window(dsp.RND, length)
        self.assertEqual(length, len(win))

    def test_bad_window_type(self):
        length = random.randint(1, 1000)
        self.assertRaises(TypeError, wavetables.window, 'this is not a window type', length)

    def test_random_wavetable(self):
        length = random.randint(1, 1000)
        wt = wavetables.wavetable(dsp.RND, length)
        self.assertEqual(length, len(wt))

    def test_bad_wavetable_type(self):
        length = random.randint(1, 1000)
        self.assertRaises(TypeError, wavetables.wavetable, 'this is not a wavetable type', length)

