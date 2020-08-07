#cython: language_level=3

import numpy as np
cimport cython
from libc cimport math
from pippi.wavetables cimport to_wavetable, to_flag, LINEAR, HERMITE, TRUNC


@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _hermite_pos(double[:] data, double pos) nogil:
    return _hermite_point(data, pos * <double>(len(data)-1))

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _hermite_point_pw(double[:] data, double phase, double pulsewidth) nogil:
    cdef int dlength = <int>len(data)
    cdef int boundry = dlength - 1

    if dlength == 1:
        return data[0]

    elif dlength < 1 or pulsewidth == 0:
        return 0

    phase *= 1.0/pulsewidth

    cdef double frac = phase - <int>phase
    cdef int i1 = <int>phase
    cdef int i2 = i1 + 1
    cdef int i3 = i2 + 1
    cdef int i0 = i1 - 1

    cdef double y0 = 0
    cdef double y1 = 0
    cdef double y2 = 0
    cdef double y3 = 0

    if i0 >= 0:
        y0 = data[i0]

    if i1 <= boundry:
        y1 = data[i1]

    if i2 <= boundry:
        y2 = data[i2]

    if i3 <= boundry:
        y3 = data[i3]

    # This part taken from version #2 by James McCartney 
    # https://www.musicdsp.org/en/latest/Other/93-hermite-interpollation.html
    cdef double c0 = y1
    cdef double c1 = 0.5 * (y2 - y0)
    cdef double c3 = 1.5 * (y1 - y2) + 0.5 * (y3 - y0)
    cdef double c2 = y0 - y1 + c1 - c3
    return ((c3 * frac + c2) * frac + c1) * frac + c0

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _hermite_point(double[:] data, double phase) nogil:
    return _hermite_point_pw(data, phase, 1)


@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _linear_point_pw(double[:] data, double phase, double pulsewidth) nogil:
    cdef int dlength = <int>len(data)

    if dlength == 1:
        return data[0]

    elif dlength < 1 or pulsewidth == 0:
        return 0

    phase *= 1.0/pulsewidth

    cdef double frac = phase - <long>phase
    cdef long i = <long>phase
    cdef double a, b

    if i >= dlength-1:
        return 0

    a = data[i]
    b = data[i+1]

    return (1.0 - frac) * a + (frac * b)

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _linear_point(double[:] data, double phase) nogil:
    return _linear_point_pw(data, phase, 1)

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _linear_pos(double[:] data, double pos) nogil:
    return _linear_point(data, pos * <double>(len(data)-1))

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double[:] _linear(double[:] data, int length):
    cdef double[:] out = np.zeros(length)
    cdef Py_ssize_t i

    for i in range(length):
        out[i] = _linear_pos(data, <double>i/<double>(length))

    return out

cpdef double[:] linear(object data, int length):
    cdef double[:] _data = to_wavetable(data)
    return _linear(data, length)

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double[:] _trunc(double[:] data, int length):
    cdef double[:] out = np.zeros(length)
    cdef Py_ssize_t i

    for i in range(length-1):
        out[i] = _trunc_pos(data, <double>i/<double>(length-1))

    return out

cpdef double[:] trunc(object data, int length):
    cdef double[:] _data = to_wavetable(data)
    return _trunc(data, length)


@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _trunc_point(double[:] data, double phase) nogil:
    return data[<int>phase % (len(data)-1)]

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double _trunc_pos(double[:] data, double pos) nogil:
    return _trunc_point(data, pos * <double>(len(data)-1))

cpdef double trunc_point(double[:] data, double phase):
    return _trunc_point(data, phase)

cpdef double trunc_pos(double[:] data, double pos):
    return _trunc_pos(data, pos)

cpdef double linear_point(double[:] data, double phase, double pulsewidth=1):
    return _linear_point_pw(data, phase, pulsewidth)

cpdef double linear_pos(double[:] data, double pos):
    return _linear_pos(data, pos)

cdef interp_point_t get_point_interpolator(str flag):
    cdef int _flag = to_flag(flag)
    cdef interp_point_t interpolator

    if _flag == LINEAR:
        interpolator = _linear_point
    elif _flag == HERMITE:
        interpolator = _hermite_point
    elif _flag == TRUNC:
        interpolator = _trunc_point
    else:
        interpolator = _linear_point

    return interpolator

cdef interp_pos_t get_pos_interpolator(str flag):
    cdef int _flag = to_flag(flag)
    cdef interp_pos_t interpolator

    if _flag == LINEAR:
        interpolator = _linear_pos
    elif _flag == HERMITE:
        interpolator = _hermite_pos
    elif _flag == TRUNC:
        interpolator = _trunc_pos
    else:
        interpolator = _linear_pos

    return interpolator
