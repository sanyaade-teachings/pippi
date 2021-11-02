import tempfile
import random

from unittest import TestCase
from pippi import rhythm, oscs

test_eu_patterns = [
    ((6, 3, 0, None, False), [1,0,1,0,1,0]), 
    ((6, 3, 0, 2, False), [1,0,1,0,1,0]*2), 
    ((6, 3, 1, 2, False), [0,1,0,1,0,1,0,1,0,1,0,1]),
    ((6, 3, 1, 2, True), [1,0,1,0,1,0,1,0,1,0,1,0]),
    ((12, 3, 0, None, False), [1,0,0,0,1,0,0,0,1,0,0,0]), 
]

test_topositions = [
    ('xxxx', 0.25, 1, [0, 0.25, 0.5, 0.75]), 
    ('xxx.', 0.25, 1, [0, 0.25, 0.5]), 
    ('xx.x', 0.25, 1, [0, 0.25, 0.75]), 
    ('x..x', 0.25, 1, [0, 0.75]), 
    ('x[xx]xx', 0.25, 1, [0, 0.25, 0.375, 0.5, 0.75]), 
    ('x[.x]xx', 0.25, 1, [0, 0.375, 0.5, 0.75]), 
    ('x[.x][xx].', 0.25, 1, [0, 0.375, 0.5, 0.625]), 
    ('x[.x][xx]x', 0.25, 1, [0, 0.375, 0.5, 0.625, 0.75]), 

    # FIXME add recursive beat divs
    #('[xx][x[xx]]', 0.5, 1, [0, 0.25, 0.5, 0.75, 0.875]),
]

class TestRhythm(TestCase):
    def test_eu(self):
        for args, result in test_eu_patterns:
            pattern = rhythm.eu(*args)
            self.assertEqual(pattern, result)

    def test_topositions(self):
        for pattern, beat, length, result in test_topositions:
            pattern = rhythm.onsets(pattern, beat, length)
            self.assertEqual(pattern, result)

    def test_seq_score(self):
        def makebloop(ctx):
            return oscs.SineOsc(freq=200).play(0.2).env('pluckout') * 0.1

        bpm = 60 / 88

        bloop_patterns = {
            'a': 'xx..',
            'b': '.x.x',
        }

        score = {
            'a': {
                'bloops': 'aaaa',
            },
            'seq': 'aa',
        }

        seq = rhythm.Seq(bpm)
        seq.add('bloops', pattern=bloop_patterns, callback=makebloop)

        out = seq.score(score)

        out.write('tests/renders/rhythm-seq-score.wav')
