from unittest import TestCase
from pippi import interpolation
import numpy as np
import random

class TestInterpolation(TestCase):
    def test_interpolate_linear(self):
        for _ in range(10):
            wtsize = random.randint(1, 100)
            npoints = random.randint(2, 100) 
            wt = np.array([ random.random() for _ in range(wtsize) ], dtype='d')

            #print(wtsize, npoints)

            points = []
            for i in range(npoints):
                points += [ interpolation.linear_point(wt, i/(npoints-1)) ]

            lpoints = [ p for p in interpolation.linear(wt, npoints) ]

            self.assertEqual(points, lpoints)

