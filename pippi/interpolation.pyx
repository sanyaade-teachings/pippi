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

    for fi in range(length):
        x = fi / float(length)
        pos = <int>(x * datalen)

        i0 = (pos - 1) % datalen
        i1 = (pos + 1) % datalen
        i2 = (pos + 2) % datalen

        out[fi] = get_sample(x, data[i0], data[pos], data[i1], data[i2])

    return out

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double[:] _linear_inner(double[:] data, double[:] out, int length) nogil:
    cdef Py_ssize_t i
    cdef int inputlength = len(data)

    if inputlength <= 1:
        return out

    for i in range(length):
        out[i] = _linear_point(data, <double>i/(length-1))

    return out

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double[:] _linear(double[:] data, int length):
    cdef double[:] out = np.zeros(length) 
    return _linear_inner(data, out, length)

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _linear_point(double[:] data, double pos) nogil:
    cdef int length = <int>len(data)
    pos = pos * <double>(length-1)
    cdef double frac = pos - <int>pos
    cdef int i = <int>pos % length
    cdef int n
    cdef double a, b

    a = data[i]
    n = i+1
    if n < length:
        b = data[n]
    else:
        b = a

    return (1.0 - frac) * a + (frac * b)

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _trunc_point(double[:] data, double pos) nogil:
    cdef int length = <int>len(data)
    cdef int i = <int>(pos*length)
    return data[i]

cpdef double linear_point(double[:] data, double pos):
    return _linear_point(data, pos)

cpdef double[:] linear(data, int length):
    if isinstance(data, list):
        data = np.asarray(data, dtype='d')
    return _linear(data, length)

def hermite(data, int length):
    if isinstance(data, list):
        data = np.asarray(data, dtype='d')
    return _hermite(data, length)

