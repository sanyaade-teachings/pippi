import collections
import random

import soundfile
import numpy as np

from libc.stdlib cimport malloc, realloc, calloc, free
from libc.stdlib cimport rand
from libc cimport math

from . cimport interpolation
from .soundbuffer cimport SoundBuffer

cdef int SINE = 0
cdef int COS = 1
cdef int TRI = 2
cdef int SAW = 3
cdef int PHASOR = SAW
cdef int RSAW = 4
cdef int HANN = 5
cdef int HAMM = 6
cdef int BLACK = 7
cdef int BLACKMAN = 7
cdef int BART = 8
cdef int BARTLETT = 8
cdef int KAISER = 9
cdef int SQUARE = 10
cdef int RND = 11

cdef int LEN_WINDOWS = 9
cdef int *ALL_WINDOWS = [
            SINE, 
            TRI, 
            SAW,
            RSAW,
            HANN,
            HAMM,
            BLACK,
            BART,
            KAISER
        ]

cdef int LEN_WAVETABLES = 6
cdef int *ALL_WAVETABLES = [
            SINE, 
            COS,
            TRI,
            SAW,
            RSAW,
            SQUARE
        ]

cdef double SQUARE_DUTY = 0.5

cdef class Wavetable:
    cdef double* data
    def __cinit__(self, int length):
        self.data = <double*>calloc(length, sizeof(double))

    cdef void resize(self, int length):
        self.data = <double*>realloc(self.data, length * sizeof(double))

    def __dealloc__(self):
        free(self.data)

cdef Wavetable _window2(int window_type, int length):
    cdef Wavetable wt = Wavetable(length)
    cdef int i = 0
    for i in range(length):
        wt.data[i] = i / length

    return wt

cdef double[:] _window(int window_type, int length):
    cdef double[:] wt

    if window_type == RND:
        window_type = ALL_WINDOWS[rand() % LEN_WINDOWS]
        wt = _window(window_type, length)

    elif window_type == SINE:
        wt = np.sin(np.linspace(0, np.pi, length, dtype='d'))

    elif window_type == TRI:
        wt = np.abs(np.abs(np.linspace(0, 2, length, dtype='d') - 1) - 1)

    elif window_type == SAW:
        wt = np.linspace(0, 1, length, dtype='d')

    elif window_type == RSAW:
        wt = np.linspace(1, 0, length, dtype='d')

    elif window_type == HANN:
        wt = np.hanning(length)

    elif window_type == HAMM:
        wt = np.hamming(length)

    elif window_type == BART:
        wt = np.bartlett(length)

    elif window_type == BLACK:
        wt = np.blackman(length)

    elif window_type == KAISER:
        wt = np.kaiser(length, 0)

    else:
        wt = window(SINE, length)

    return wt

def window(int window_type, int length, double[:] data=None):
    if data is not None:
        return interpolation._linear(data, length)

    return _window(window_type, length)

cdef double[:] _wavetable(int wavetable_type, int length):
    cdef double[:] wt

    if wavetable_type == RND:
        wavetable_type = ALL_WAVETABLES[rand() % LEN_WAVETABLES]
        wt = _wavetable(wavetable_type, length)

    elif wavetable_type == SINE:
        wt = np.sin(np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False))

    elif wavetable_type == COS:
        wt = np.cos(np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False))

    elif wavetable_type == TRI:
        tmp = np.abs(np.linspace(-1, 1, length, dtype='d', endpoint=False))
        tmp = np.abs(tmp)
        wt = (tmp - tmp.mean()) * 2

    elif wavetable_type == SAW:
        wt = np.linspace(-1, 1, length, dtype='d', endpoint=False)

    elif wavetable_type == RSAW:
        wt = np.linspace(1, -1, length, dtype='d', endpoint=False)

    elif wavetable_type == SQUARE:
        tmp = np.zeros(length, dtype='d')
        duty = int(length * SQUARE_DUTY)
        tmp[:duty] = 1
        tmp[duty:] = -1
        wt = tmp

    else:
        wt = _wavetable(SINE, length)

    return wt

cpdef double[:] wavetable(int wavetable_type, int length, double[:] data=None):
    if data is not None:
        return interpolation._linear(data, length)

    return _wavetable(wavetable_type, length)

cpdef double[:] fromfile(unicode filename, int length):
    wt, _ = soundfile.read(filename, dtype='d')
    if len(wt) == length:
        return wt

    return interpolation._linear(wt, length)


