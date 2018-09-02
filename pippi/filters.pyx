# cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer
from pippi.interpolation cimport _linear_point
from pippi cimport wavetables
from pippi.dsp cimport _mag
from pippi.fx cimport _norm
cimport cython
from cpython cimport bool
import numpy as np
import random

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double[:,:] _fir(double[:,:] snd, double[:,:] out, double[:] impulse, int norm):
    cdef int i = 0
    cdef int c = 0
    cdef int j = 0
    cdef int framelength = len(snd)
    cdef int channels = snd.shape[1]
    cdef int impulselength = len(impulse)
    cdef double sample = 0
    cdef int delayindex = 0
    cdef double maxval     

    if norm > 0:
        maxval = _mag(snd)

    for i in range(framelength):
        for c in range(channels):
            sample = 0
            for j in range(impulselength):
                delayindex = i - j
                if delayindex > 0:
                    sample += impulse[j] * snd[delayindex,c]

            out[i,c] = sample

    if norm > 0:
        return _norm(out, maxval)
    else:
        return out

cpdef SoundBuffer fir(SoundBuffer snd, double[:] impulse, bool normalize=True):
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    cdef int _normalize = 1 if normalize else 0
    snd.frames = _fir(snd.frames, out, impulse, normalize)
    return snd
