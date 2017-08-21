import numpy as np
cimport cython

ctypedef double (*get_sample_t)(double x, double y0, double y1, double y2, double y3)

@cython.boundscheck(False)
@cython.wraparound(False)
cdef double hermite_get_sample(double x, double y0, double y1, double y2, double y3):
    cdef double c0 = y1
    cdef double c1 = 0.5 * (y2 - y0)
    cdef double c3 = 1.5 * (y1 - y2) + 0.5 * (y3 - y0)
    cdef double c2 = y0 - y1 + c1 - c3

    return ((c3 * x + c2) * x + c1) * x + c0

cdef public double[:] _hermite(double[:] data, int length):
    cdef double x, y0, y1, y2, y3 = 0
    cdef int fi = 0
    cdef double[:] out = np.zeros(length)
    cdef get_sample_t get_sample = hermite_get_sample

    for fi in range(length):
        x = length / <double>(fi + 1) 
        y0 = data[fi - 1]
        y1 = data[fi]
        y2 = data[fi + 1]
        y3 = data[fi + 2]

        out[fi] = get_sample(x, y0, y1, y2, y3)

    return out

cdef public double[:] _linear(double[:] data, int length):
    cdef double[:] out = np.zeros(length) 

    cdef int i = 0
    cdef int readindex = 0
    cdef int inputlength = len(data)
    cdef double phase = 0
    cdef double val, nextval, frac

    if inputlength <= 1:
        return out

    for i in range(length):
        readindex = int(phase) % inputlength

        val = data[readindex]

        try:
            nextval = data[readindex + 1]
        except IndexError:
            nextval = data[0]

        frac = phase - int(phase)

        val = (1.0 - frac) * val + frac * nextval

        out[i] = val

        phase += inputlength * (1.0 / length)

    assert len(out) == length

    return out

def linear(data, int length):
    if isinstance(data, list):
        data = np.asarray(data, dtype='d')
    return _linear(data, length)

def hermite(data, int length):
    if isinstance(data, list):
        data = np.asarray(data, dtype='d')
    return _hermite(data, length)

