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

cdef class Pulsar:
    """ Pulsar synthesis
    """
    def __cinit__(
            self, 
            object wavetable=None, 
            object freq=440.0, 
            object amp=1, 
            object pulsewidth=1,
            double phase=0, 

            object window=None, 

            int wtsize=4096,
            int channels=DEFAULT_CHANNELS,
            int samplerate=DEFAULT_SAMPLERATE,
        ):

        self.freq = wavetables.to_wavetable(freq, self.wtsize)
        self.amp = amp
        self.phase = phase

        self.channels = channels
        self.samplerate = samplerate
        self.wtsize = wtsize

        self.pulsewidth = wavetables.to_wavetable(pulsewidth, self.wtsize)
        self.wavetable = wavetables.to_wavetable(wavetable, self.wtsize)
        self.window = wavetables.to_wavetable(window, self.wtsize)

        cdef int i = 0
        for i in range(self.wtsize):
            self.wavetable[i] *= self.window[i]


    def play(self, length):
        framelength = <int>(length * self.samplerate)
        return self._play(framelength)

    cdef object _play(self, int length):
        cdef double[:,:] out = np.zeros((length, self.channels), dtype='d')

        cdef int i = 0
        cdef int wtsize = self.wtsize
        cdef double sample
        cdef double pulsewidth
        cdef double freq

        cdef double phase_inc = (1.0 / self.samplerate) * wtsize

        for i in range(length):
            freq = interpolation._linear_point(self.freq, self.phase)
            pulsewidth = interpolation._linear_point(self.pulsewidth, self.phase)
            sample = interpolation._linear_point(self.wavetable, self.phase)
            sample *= interpolation._linear_point(self.amp, self.phase) 

            self.phase += freq * phase_inc

            for channel in range(self.channels):
                out[i][channel] = sample

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

