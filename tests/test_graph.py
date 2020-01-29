from unittest import TestCase
from pippi import wavetables, dsp, graph

class TestGraph(TestCase):
    def test_insets(self):
        wt1 = wavetables.seesaw('rnd', 4096, dsp.rand(0, 1))
        wt1_graph = wt1.graph()

        wt2 = wavetables.seesaw('rnd', 4096, dsp.rand(0, 1))
        wt2_graph = wt2.graph()

        wt3 = wavetables.seesaw('rnd', 4096, dsp.rand(0, 1))
        wt3_graph = wt3.graph()

        snd = dsp.read('tests/sounds/linux.wav')
        snd.graph('tests/renders/graph_insets.png', insets=[wt1_graph, wt2_graph, wt3_graph], stroke=3, width=1200, height=500)


    def test_range(self):
        wt = dsp.win('sine')
        wt.graph('tests/renders/graph_sine_range--1-1.png', y=(-1, 1))
        wt.graph('tests/renders/graph_sine_range--2-2.png', y=(-2, 2))
        wt.graph('tests/renders/graph_sine_range-0-1.png', y=(0, 1))
        wt.graph('tests/renders/graph_sine_range-0.5-2.png', y=(0.5, 2))

        wt = dsp.win('sinc')
        wt.graph('tests/renders/graph_sinc_range--0.2-1.png', y=(-0.25, 1))

    def test_labels(self):
        wt = dsp.win('sinc')
        wt.graph('tests/renders/graph_sinc_label.png', y=(-0.25, 1), label="Default label")
        wt.graph('tests/renders/graph_sinc_label_top.png', y=(-0.25, 1), label_top="Label top")
        wt.graph('tests/renders/graph_sinc_label_bottom.png', y=(-0.25, 1), label_bottom="Label bottom")
        wt.graph('tests/renders/graph_sinc_label_long.png', y=(-0.25, 1), label_bottom="A long bit of text & numbers 1,2,3")
