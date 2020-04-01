#cython: language_level=3

from libc.stdlib cimport malloc, free

import numpy as np
cimport numpy as np

from pippi.interpolation cimport _linear_pos
from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport to_window

np.import_array()

cdef double[:] _conv(double[:] x, double[:] y):
    cdef size_t i = 0
    cdef size_t x_length = len(x)
    cdef size_t y_length = len(y)
    cdef size_t length = x_length + y_length + 1

    cdef double* _x = <double*>malloc(sizeof(double) * length)
    cdef double* _y = <double*>malloc(sizeof(double) * length)
    cdef double* _out = <double*>malloc(sizeof(double) * length)

    for i in range(length):
        if i < x_length:
            _x[i] = x[i]
        else:
            _x[i] = 0

        if i < y_length:
            _y[i] = y[i]
        else:
            _y[i] = 0

    Fft_convolveReal(_x, _y, _out, length)

    cdef double[:] out = np.zeros(length, dtype='d')

    for i in range(length):
        out[i] = _out[i]

    free(_x)
    free(_y)
    free(_out)

    return out

cpdef double[:,:] conv(double[:,:] src, double[:,:] impulse):
    cdef int channels = src.shape[1]
    cdef size_t length = len(src) + len(impulse) + 1
    cdef double[:,:] out = np.zeros((length, channels), dtype='d')

    for c in range(channels):
        out[:,c] = _conv(src[:,c], impulse[:,c])

    return out

cpdef tuple transform(SoundBuffer snd):
    cdef double[:,:] src = snd.frames
    cdef int channels = src.shape[1]
    cdef size_t length = len(src)

    cdef double[:,:] rout = np.zeros((length, channels), dtype='d')
    cdef double[:,:] iout = np.zeros((length, channels), dtype='d')

    cdef size_t i = 0
    cdef size_t c = 0
    cdef double pos = 0

    cdef double* _real = <double*>malloc(sizeof(double) * length)
    cdef double* _imag = <double*>malloc(sizeof(double) * length)

    for c in range(channels):
        for i in range(length):
            _real[i] = src[i,c]
            _imag[i] = 1

        Fft_transform(_real, _imag, length)

        for i in range(length):
            rout[i,c] = _real[i]

        for i in range(length):
            iout[i,c] = _imag[i]

    free(_real)
    free(_imag)

    return (
        SoundBuffer(rout, channels=channels, samplerate=snd.samplerate), 
        SoundBuffer(iout, channels=channels, samplerate=snd.samplerate)
    )


cpdef SoundBuffer itransform(SoundBuffer real, SoundBuffer imag):
    cdef int channels = real.channels
    cdef size_t length = len(real)

    cdef double[:,:] out = np.zeros((length, channels), dtype='d')

    cdef size_t i = 0
    cdef size_t c = 0
    cdef double pos = 0

    cdef double* _real = <double*>malloc(sizeof(double) * length)
    cdef double* _imag = <double*>malloc(sizeof(double) * length)

    for c in range(channels):
        for i in range(length):
            _real[i] = real.frames[i,c]
            _imag[i] = imag.frames[i,c]

        Fft_inverseTransform(_real, _imag, length)

        for i in range(length):
            out[i,c] = _real[i]

    free(_real)
    free(_imag)

    return SoundBuffer(out, channels=channels, samplerate=real.samplerate)



