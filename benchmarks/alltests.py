""" 
Profile the test suite:

    python -m cProfile unittest.profile benchmarks/alltests.py

Visualize the profiling information:

    snakeviz unittest.profile
"""
import unittest

loader = unittest.TestLoader()
tests = loader.discover('tests')
runner = unittest.runner.TextTestRunner()
runner.run(tests)
