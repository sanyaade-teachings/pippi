# cython: language_level=3

import numpy as np
cimport cython
from libc cimport math

ctypedef double (*get_sample_t)(double x, double y0, double y1, double y2, double y3)

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double hermite_get_sample(double x, double y0, double y1, double y2, double y3):
    """ Taken from version #2 by James McCartney 
        http://musicdsp.org/showone.php?id=93
    """
    cdef double c0 = y1
    cdef double c1 = 0.5 * (y2 - y0)
    cdef double c3 = 1.5 * (y1 - y2) + 0.5 * (y3 - y0)
    cdef double c2 = y0 - y1 + c1 - c3

    return ((c3 * x + c2) * x + c1) * x + c0

cdef double[:] _hermite(double[:] data, int length):
    cdef int fi = 0
    cdef double[:] out = np.zeros(length)
    cdef get_sample_t get_sample = hermite_get_sample
    cdef int datalen = len(data)
    cdef int i0, i1, i2 = 0
    cdef int pos = 0
    cdef double x = 0

    for fi in range(length):
        x = fi / <double>length
        pos = <int>(x * datalen)

        i0 = (pos - 1) % datalen
        i1 = (pos + 1) % datalen
        i2 = (pos + 2) % datalen

        out[fi] = get_sample(x, data[i0], data[pos], data[i1], data[i2])

    return out

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _linear_point(double[:] data, double phase) nogil:
    cdef int dlength = <int>len(data)
    if dlength == 1:
        return data[0]

    elif dlength < 1:
        return 0

    cdef double frac = phase - <int>phase
    cdef int i = <int>phase % (len(data)-1)
    cdef double a, b

    a = data[i]
    b = data[i+1]

    return (1.0 - frac) * a + (frac * b)

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _linear_pos(double[:] data, double pos) nogil:
    return _linear_point(data, pos * <double>(len(data)-1))

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double[:] _linear(double[:] data, int length):
    cdef double[:] out = np.zeros(length)
    cdef Py_ssize_t i

    for i in range(length-1):
        out[i] = _linear_point(data, <double>i/<double>(length-1))

    return out

cpdef double[:] linear(double[:] data, int length):
    return _linear(data, length)

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _trunc_point(double[:] data, double phase) nogil:
    return data[<int>phase % (len(data)-1)]

cpdef double linear_point(double[:] data, double pos):
    return _linear_point(data, pos)


