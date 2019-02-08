import tempfile
import random
import re

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

    def test_wtclass(self):
        wt1 = dsp.wt(dsp.RND, wtsize=4096)
        wt2 = dsp.wt(dsp.TRI, wtsize=4096)
        wt3 = dsp.wt([ random.random()+0.001 for _ in range(1000) ], wtsize=1000)

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

    def test_graph_sine_wavetable(self):
        wt = dsp.wt(dsp.SINE)
        wt.graph('tests/renders/graph_sine_wavetable.png', stroke=3)

    def test_graph_cos_wavetable(self):
        wt = dsp.wt(dsp.COS)
        wt.graph('tests/renders/graph_cos_wavetable.png', stroke=3)

    def test_graph_tri_wavetable(self):
        wt = dsp.wt(dsp.TRI)
        wt.graph('tests/renders/graph_tri_wavetable.png', stroke=3)

    def test_graph_saw_wavetable(self):
        wt = dsp.wt(dsp.SAW)
        wt.graph('tests/renders/graph_saw_wavetable.png', stroke=3)

    def test_graph_rsaw_wavetable(self):
        wt = dsp.wt(dsp.RSAW)
        wt.graph('tests/renders/graph_rsaw_wavetable.png', stroke=3)

    def test_graph_square_wavetable(self):
        wt = dsp.wt(dsp.SQUARE)
        wt.graph('tests/renders/graph_square_wavetable.png', stroke=3)

    def test_graph_sinein_window(self):
        wt = dsp.wt(dsp.SINEIN, window=True)
        wt.graph('tests/renders/graph_sinein_window.png', stroke=3)

    def test_graph_sineout_window(self):
        wt = dsp.wt(dsp.SINEOUT, window=True)
        wt.graph('tests/renders/graph_sineout_window.png', stroke=3)

    def test_graph_cos_window(self):
        wt = dsp.wt(dsp.COS, window=True)
        wt.graph('tests/renders/graph_cos_window.png', stroke=3)

    def test_graph_tri_window(self):
        wt = dsp.wt(dsp.TRI, window=True)
        wt.graph('tests/renders/graph_tri_window.png', stroke=3)

    def test_graph_saw_window(self):
        wt = dsp.wt(dsp.SAW, window=True)
        wt.graph('tests/renders/graph_saw_window.png', stroke=3)

    def test_graph_rsaw_window(self):
        wt = dsp.wt(dsp.RSAW, window=True)
        wt.graph('tests/renders/graph_rsaw_window.png', stroke=3)

    def test_graph_hann_window(self):
        wt = dsp.wt(dsp.HANN, window=True)
        wt.graph('tests/renders/graph_hann_window.png', stroke=3)

    def test_graph_hannin_window(self):
        wt = dsp.wt(dsp.HANNIN, window=True)
        wt.graph('tests/renders/graph_hannin_window.png', stroke=3)

    def test_graph_hannout_window(self):
        wt = dsp.wt(dsp.HANNOUT, window=True)
        wt.graph('tests/renders/graph_hannout_window.png', stroke=3)

    def test_graph_hamm_window(self):
        wt = dsp.wt(dsp.HAMM, window=True)
        wt.graph('tests/renders/graph_hamm_window.png', stroke=3)

    def test_graph_black_window(self):
        wt = dsp.wt(dsp.BLACK, window=True)
        wt.graph('tests/renders/graph_black_window.png', stroke=3)

    def test_graph_bart_window(self):
        wt = dsp.wt(dsp.BART, window=True)
        wt.graph('tests/renders/graph_bart_window.png', stroke=3)

    def test_graph_kaiser_window(self):
        wt = dsp.wt(dsp.KAISER, window=True)
        wt.graph('tests/renders/graph_kaiser_window.png', stroke=3)

    def test_graph_sinc_window(self):
        wt = dsp.wt(dsp.SINC, window=True)
        wt.graph('tests/renders/graph_sinc_window.png', stroke=3)


