from unittest import TestCase
from pippi import dsp, hyperupic, oscs, tune

class TestHyperUPIC(TestCase):
    def test_get_partials_from_image(self):
        partials = hyperupic.partials('tests/images/louis.jpg', 4)

        for i, p in enumerate(partials):
            for j, c in enumerate(p):
                c.graph('tests/renders/hyperupic-partial-%02d-%02d.png' % (i, j), y=(0,1))

    def test_render_image(self):
        length = 30
        degrees = list(range(1,33))
        freqs = tune.degrees(degrees, octave=2)

        out = hyperupic.render('tests/images/louis.jpg', freqs, length)
        out.write('tests/renders/hyperupic-render.wav')

