# cython: language_level=3

import numbers
import numpy as np
from pippi.soundbuffer cimport SoundBuffer
from pippi cimport wavetables
from pippi cimport interpolation

cimport cython
from cpython.array cimport array, clone

cdef inline int DEFAULT_SAMPLERATE = 44100
cdef inline int DEFAULT_CHANNELS = 2
cdef inline double MIN_PULSEWIDTH = 0.0001

cdef class Osc:
    """ wavetable osc
    """
    def __cinit__(
            self, 
            object wavetable=None, 
            object freq=440.0, 
            object amp=1.0, 
            double phase=0, 

            int wtsize=4096,
            int channels=DEFAULT_CHANNELS,
            int samplerate=DEFAULT_SAMPLERATE,
        ):

        self.freq = wavetables.to_wavetable(freq, self.wtsize)
        self.amp = wavetables.to_window(amp, self.wtsize)
        self.phase = phase

        self.channels = channels
        self.samplerate = samplerate
        self.wtsize = wtsize

        self.wavetable = wavetables.to_wavetable(wavetable, self.wtsize)

    def play(self, length):
        framelength = <int>(length * self.samplerate)
        return self._play(framelength)

    cdef object _play(self, int length):
        cdef int i = 0
        cdef double sample, freq, amp

        cdef double phase_inc = (1.0 / self.samplerate) * self.wtsize
        cdef double[:,:] out = np.zeros((length, self.channels), dtype='d')

        for i in range(length):
            freq = interpolation._linear_point(self.freq, self.phase)
            amp = interpolation._linear_point(self.amp, self.phase)
            sample = interpolation._linear_point(self.wavetable, self.phase) * amp

            self.phase += freq * phase_inc

            for channel in range(self.channels):
                out[i][channel] = sample

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

