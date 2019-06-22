import tempfile
import random
import re

from unittest import TestCase
from pippi import lists, dsp

class TestWavetables(TestCase):
    def test_scaled_list(self):
        source = [1,2,3]
        target = [3,4,5]
        result = dsp.scale(source, 1, 3, 3, 5)
        self.assertEqual(result, target)


