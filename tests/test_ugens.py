from unittest import TestCase
from pippi import dsp, fx, ugens
import numpy as np

class TestUgens(TestCase):
    def test_ugen_tape(self):
        buf = dsp.read('tests/sounds/living.wav')

        graph = ugens.Graph()
        graph.add_node('s1', 'sine', freq=100)
        graph.add_node('s2', 'sine', freq=0.1)
        graph.add_node('t', 'tape', buf=buf)

        graph.connect('s1.output', 't.speed', 0.1, 1)
        graph.connect('s2.output', 's1.freq', 10, 2000)
        graph.connect('t.output', 'main.output', mult=0.5)

        out = graph.render(10)
        out = fx.norm(out, 1)
        out.write('tests/renders/ugens_tape.wav')

    def test_ugen_feedback(self):
        graph = ugens.Graph()
        graph.add_node('s0', 'sine', freq=100)
        graph.add_node('s1', 'sine', freq=100)
        graph.add_node('m0', 'mult')

        graph.connect('s0.output', 's1.freq', 1, 200)
        graph.connect('s1.output', 's0.freq', 1, 200)
        graph.connect('s0.freq', 's1.phase', 0, 1, 1, 200)

        graph.connect('s0.output', 'm0.a')
        graph.connect('s1.output', 'm0.b')
        graph.connect('m0.output', 'main.output', mult=0.5)

        out = graph.render(10)
        out = fx.norm(out, 1)
        out.write('tests/renders/ugens_feedback.wav')

    def test_ugen_sine(self):
        graph = ugens.Graph()
        graph.add_node('s0b', 'sine', freq=0.1)
        graph.add_node('s0a', 'sine', freq=0.1)
        graph.add_node('s0', 'sine', freq=100)
        graph.add_node('s1', 'sine')
        graph.add_node('s2', 'sine')

        graph.connect('s0b.output', 's0a.freq', 0.1, 200)
        graph.connect('s0a.output', 's0.freq', 100, 300)
        graph.connect('s0.output', 's1.freq', 0.1, 1)
        graph.connect('s0.output', 'main.output', mult=0.1)
        graph.connect('s1.output', 's2.freq', 60, 100)
        graph.connect('s2.output', 'main.output', mult=0.2)
        graph.connect('s2.freq', 's0b.freq', 0.15, 0.5, 60, 100)

        out = graph.render(10)
        out = fx.norm(out, 1).taper(0.1)
        out.write('tests/renders/ugens_sine.wav')
