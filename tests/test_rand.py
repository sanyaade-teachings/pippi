from unittest import TestCase
from pippi import dsp

methods = ['normal', 'logistic', 'lorenz', 'lorenzX', 'lorenzY', 'lorenzZ']

class TestRand(TestCase):
    def test_rand_1000(self):
        dsp.seed(1000)
        for m in methods:
            dsp.randmethod(m)
            values = []
            for _ in range(1000):
                values += [dsp.rand()]
            dsp.win(values).graph('tests/renders/rand_%s_1000values.png' % m) 
        # teardown
        dsp.randmethod('normal')
