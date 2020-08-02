#cython: language_level=3

import numbers
import numpy as np
from pippi.soundbuffer cimport SoundBuffer
from pippi cimport rand
from pippi cimport wavetables
from pippi cimport interpolation

cimport cython

cdef inline int DEFAULT_SAMPLERATE = 44100
cdef inline int DEFAULT_CHANNELS = 2
cdef inline double MIN_PULSEWIDTH = 0.0001


cdef class DSS:
    """ Dynamic stochastic synthesis
    """
    def __cinit__(
            self, 
            int points=10,
            object freq=440.0, 
            object xwidth=0.001, 
            object ywidth=0.01, 
            object amp=1.0, 
            double phase=0, 

            int wtsize=4096,
            int channels=DEFAULT_CHANNELS,
            int samplerate=DEFAULT_SAMPLERATE,
        ):

        self.wtsize = wtsize
        self.numpoints = points
        self.points = Breakpoint(points, wtsize)
        self.freq = wavetables.to_wavetable(freq, wtsize)
        self.xwidth = wavetables.to_wavetable(xwidth, wtsize)
        self.ywidth = wavetables.to_wavetable(ywidth, wtsize)
        self.amp = wavetables.to_window(amp, wtsize)

        self.wt_phase = phase
        self.freq_phase = phase
        self.xwidth_phase = phase
        self.ywidth_phase = phase
        self.amp_phase = phase

        self.channels = channels
        self.samplerate = samplerate

        self.wavetable = self.points.towavetable().data

    def play(self, length):
        framelength = <int>(length * self.samplerate)
        return self._play(framelength)

    cdef object _play(self, int length):
        cdef int i = 0
        cdef double sample, freq, amp, width
        cdef double ilength = 1.0 / length

        cdef int freq_boundry = max(len(self.freq)-1, 1)
        cdef int xwidth_boundry = max(len(self.xwidth)-1, 1)
        cdef int ywidth_boundry = max(len(self.ywidth)-1, 1)
        cdef int amp_boundry = max(len(self.amp)-1, 1)
        cdef int wt_boundry = max(len(self.wavetable)-1, 1)

        cdef double freq_phase_inc = ilength * freq_boundry
        cdef double xwidth_phase_inc = ilength * xwidth_boundry
        cdef double ywidth_phase_inc = ilength * ywidth_boundry
        cdef double amp_phase_inc = ilength * amp_boundry

        cdef double wt_phase_inc = (1.0 / self.samplerate) * self.wtsize
        cdef double[:,:] out = np.zeros((length, self.channels), dtype='d')

        for i in range(length):
            freq = interpolation._linear_point(self.freq, self.freq_phase)
            xwidth = interpolation._linear_point(self.xwidth, self.xwidth_phase)
            ywidth = interpolation._linear_point(self.ywidth, self.ywidth_phase)
            amp = interpolation._linear_point(self.amp, self.amp_phase)
            sample = interpolation._hermite_point(self.wavetable, self.wt_phase) * amp

            self.freq_phase += freq_phase_inc
            self.xwidth_phase += xwidth_phase_inc
            self.ywidth_phase += ywidth_phase_inc
            self.amp_phase += amp_phase_inc
            self.wt_phase += freq * wt_phase_inc

            if self.wt_phase >= wt_boundry:
                self.points.drink(xwidth, ywidth, -1, 1)
                self.wavetable = self.points.towavetable().data

            while self.wt_phase >= wt_boundry:
                self.wt_phase -= wt_boundry

            while self.amp_phase >= amp_boundry:
                self.amp_phase -= amp_boundry

            while self.freq_phase >= freq_boundry:
                self.freq_phase -= freq_boundry

            while self.xwidth_phase >= xwidth_boundry:
                self.xwidth_phase -= xwidth_boundry

            while self.ywidth_phase >= ywidth_boundry:
                self.ywidth_phase -= ywidth_boundry

            for channel in range(self.channels):
                out[i][channel] = sample

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

