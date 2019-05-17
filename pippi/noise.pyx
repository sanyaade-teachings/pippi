#cython: language_level=3

from pippi.dsp cimport rand
from pippi.defaults cimport DEFAULT_CHANNELS, DEFAULT_SAMPLERATE
from pippi.wavetables cimport to_wavetable
from pippi.interpolation cimport _linear_point
from pippi.soundbuffer cimport SoundBuffer

import numpy as np
cimport numpy as np

cdef double[:,:] _bln(double[:,:] out, double[:] wt, unsigned int length, double minfreq, double maxfreq, int channels, int samplerate):
    cdef double phase=0, sample=0
    cdef int i=0, c=0
    cdef double isamplerate = 1.0/samplerate
    cdef double freq = rand(minfreq, maxfreq)
    cdef int wt_length = len(wt)
    cdef int wt_boundry = max(wt_length-1, 1)

    for i in range(length):
        sample = _linear_point(wt, phase)

        for c in range(channels):
            out[i,c] = sample

        phase += isamplerate * wt_boundry * freq

        if phase >= wt_boundry:
            freq = rand(minfreq, maxfreq)

        while phase >= wt_boundry:
            phase -= wt_boundry

    return out

cpdef SoundBuffer bln(object wt, double length, double minfreq, double maxfreq, channels=DEFAULT_CHANNELS, samplerate=DEFAULT_SAMPLERATE):
    cdef double[:] _wt = to_wavetable(wt)
    cdef unsigned int framelength = <unsigned int>(length * samplerate)
    cdef double[:,:] out = np.zeros((framelength, channels), dtype='d')
    return SoundBuffer(_bln(out, _wt, framelength, minfreq, maxfreq, channels, samplerate), channels=channels, samplerate=samplerate)
