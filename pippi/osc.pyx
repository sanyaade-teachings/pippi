# cython: language_level=3

import numbers
import numpy as np
from pippi.soundbuffer cimport SoundBuffer
from pippi cimport wavetables
from pippi cimport interpolation

cimport cython
cdef inline int DEFAULT_SAMPLERATE = 44100
cdef inline int DEFAULT_CHANNELS = 2
cdef inline double MIN_PULSEWIDTH = 0.0001

cdef class Osc:
    """ simple wavetable osc with linear interpolation
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

        cdef double wt_phase = self.phase
        cdef double freq_phase = self.phase
        cdef double amp_phase = self.phase

        cdef double ilength = 1.0 / length

        cdef int freq_boundry = max(len(self.freq)-1, 1)
        cdef int amp_boundry = max(len(self.amp)-1, 1)
        cdef int wt_boundry = max(len(self.wavetable)-1, 1)

        cdef double freq_phase_inc = ilength * freq_boundry
        cdef double amp_phase_inc = ilength * amp_boundry

        cdef double wt_phase_inc = (1.0 / self.samplerate) * self.wtsize
        cdef double[:,:] out = np.zeros((length, self.channels), dtype='d')

        for i in range(length):
            freq = interpolation._linear_point(self.freq, freq_phase)
            amp = interpolation._linear_point(self.amp, amp_phase)
            sample = interpolation._linear_point(self.wavetable, wt_phase) * amp

            freq_phase += freq_phase_inc
            amp_phase += amp_phase_inc
            wt_phase += freq * wt_phase_inc

            while wt_phase >= wt_boundry:
                wt_phase -= wt_boundry

            while amp_phase >= amp_boundry:
                amp_phase -= amp_boundry

            while freq_phase >= freq_boundry:
                freq_phase -= freq_boundry

            for channel in range(self.channels):
                out[i][channel] = sample

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

