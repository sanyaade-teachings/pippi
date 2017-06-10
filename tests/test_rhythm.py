import tempfile
import random

from unittest import TestCase
from pippi import rhythm

test_patterns = [
    ((8, 1, 0, None, False), [1,1,1,1,1,1,1,1]), 
    ((8, 2, 0, None, False), [1,0,1,0,1,0,1,0]), 
    ((8, 3, 0, None, False), [1,0,0,1,0,0,1,0]), 
    ((8, 4, 0, None, False), [1,0,0,0,1,0,0,0]), 
    ((8, 5, 0, None, False), [1,0,0,0,0,1,0,0]), 
    ((8, 6, 0, None, False), [1,0,0,0,0,0,1,0]), 
    ((8, 7, 0, None, False), [1,0,0,0,0,0,0,1]), 
    ((8, 8, 0, None, False), [1,0,0,0,0,0,0,0]), 

    ((8, 3, 1, None, False), [0,1,0,0,1,0,0,1]), 
    ((8, 2, 3, None, False), [0,0,0,1,0,1,0,1]), 
    ((8, 4, 1, None, False), [0,1,0,0,0,1,0,0]), 

    ((8, 2, 3, None, True), [1,0,1,0,1,0,0,0]), 
    ((8, 3, 0, 2, False), [1,0,0,1,0,0,1,0, 1,0,0,1,0,0,1,0]), 
]

test_topatterns = [
    ('xx  ', [1,1,0,0]), 
    ('xx- ', [1,1,0,0]), 
    ('xx. ', [1,1,0,0]), 
    ('-x..', [0,1,0,0]), 
    ('X..1*!', [1,0,0,1,1,1]), 
]

class TestRhythm(TestCase):
    def test_basic_patterns(self):
        for pattern_args, result in test_patterns:
            pattern = rhythm.pattern(*pattern_args)
            self.assertEqual(pattern, result)

    def test_basic_topatterns(self):
        for pattern, result in test_topatterns:
            pattern = rhythm.topattern(pattern)
            self.assertEqual(pattern, result)

