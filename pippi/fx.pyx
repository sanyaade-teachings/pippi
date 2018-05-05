import numpy as np

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _butter(double x, double y0, double y1, double y2, double y3):
    """ Taken from version #2 by James McCartney 
        http://musicdsp.org/showone.php?id=93
    """
    cdef double c0 = y1
    cdef double c1 = 0.5 * (y2 - y0)
    cdef double c3 = 1.5 * (y1 - y2) + 0.5 * (y3 - y0)
    cdef double c2 = y0 - y1 + c1 - c3

    return ((c3 * x + c2) * x + c1) * x + c0


def lowpass(snd, freq):
    pass


