from unittest import TestCase
from pippi import dsp

methods = ['normal', 'logistic', 'lorenz', 'lorenzX', 'lorenzY', 'lorenzZ']

def makeseedgraph(i):
    values = []
    for _ in range(1000):
        values += [dsp.rand()]
    dsp.win(values).graph('tests/renders/rand_%s_preseed.png' % i) 

class TestRand(TestCase):
    def test_rand_preseed(self):
        dsp.randmethod('normal')
        dsp.pool(makeseedgraph, params=[(i,) for i in range(10)])

    def test_rand_strseed(self):
        dsp.seed('foo')
        print(dsp.rand())
        dsp.randmethod('normal')

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
