from unittest import TestCase
from pippi import dsp
from pippi.breakpoints import Breakpoint

class TestBreakpoints(TestCase):
    def test_create_breakpoint(self):
        dsp.seed()
        b = Breakpoint()
        wt = b.towavetable()
        wt.graph('tests/renders/breakpoint_default.png')

