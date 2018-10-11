import tempfile
import random
import re

from unittest import TestCase
from pippi import wavetables, dsp, graph

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

    def test_wtclass(self):
        wt1 = dsp.wt(dsp.RND, 4096)
        wt2 = dsp.wt(dsp.TRI, 4096)
        wt3 = dsp.wt([ random.random()+0.001 for _ in range(1000) ])

        self.assertTrue(max(wt1) > 0)
        self.assertTrue(max(wt2) > 0)
        self.assertTrue(max(wt3) > 0)
        self.assertEqual(len(wt1), 4096)
        self.assertEqual(len(wt2), 4096)
        self.assertEqual(len(wt3), 1000)

    """
    def test_polyseg(self):
        score = 'sine 1,tri,0-1 rand,0.3-0.8'
        length = random.randint(100, 1000)
        for segment in score.split(' '):
            match = wavetables.SEGMENT_RE.match(segment)

     
        wt = wavetables.polyseg(score, length)

        self.assertEqual(len(wt), length)
    """

    def test_randline(self):
        numpoints = random.randint(1, 10)
        wtsize = random.randint(10, 1000)

        wt = wavetables.randline(numpoints, wtsize=wtsize)
        self.assertEqual(len(wt), wtsize)

        lowvalue = random.triangular(0, 1)
        highvalue = random.triangular(1, 5)
        wt = wavetables.randline(numpoints, 
                            lowvalue=lowvalue, 
                            highvalue=highvalue, 
                            wtsize=wtsize
                        )

        self.assertEqual(len(wt), wtsize)
        self.assertTrue(max(wt) <= highvalue)
        self.assertTrue(min(wt) >= lowvalue)


