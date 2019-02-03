#cython: language_level=3

import numbers
import numpy as np
from pippi.soundbuffer cimport SoundBuffer
from pippi cimport wavetables as wts
from pippi cimport interpolation

cdef inline int DEFAULT_SAMPLERATE = 44100
cdef inline int DEFAULT_CHANNELS = 2
cdef inline int DEFAULT_WT_LENGTH = 4096
cdef inline double MIN_PULSEWIDTH = 0.001

cdef class Pulsar2d:
    """ Pulsar synthesis with a 2d wavetable & window stack
    """
    def __cinit__(
            self, 
            object wavetables=None, 
            object windows=None, 

            object freq=440.0, 
            object pulsewidth=1,
            object amp=1.0, 
            double phase=0, 

            object wt_mod=0, 
            object win_mod=0, 

            int channels=DEFAULT_CHANNELS,
            int samplerate=DEFAULT_SAMPLERATE,
        ):

        self.freq = wts.to_wavetable(freq)
        self.amp = wts.to_window(amp)
        self.wt_mod = wts.to_window(wt_mod)
        self.win_mod = wts.to_window(win_mod)

        self.wt_phase = phase
        self.win_phase = phase
        self.freq_phase = phase
        self.pw_phase = phase
        self.amp_phase = phase

        self.channels = channels
        self.samplerate = samplerate

        self.pulsewidth = wts.to_wavetable(pulsewidth)

        cdef double[:] wt
        cdef double[:] win
        cdef double val
        cdef int i
        cdef int j

        self.wt_length = 0
        for wavetable in wavetables:
            try:
                self.wt_length = max(len(wavetable), self.wt_length)
            except TypeError:
                self.wt_length = max(DEFAULT_WT_LENGTH, self.wt_length)

        self.wt_count = len(wavetables)
        self.wavetables = np.zeros((self.wt_count, self.wt_length), dtype='d')

        for i, wavetable in enumerate(wavetables):
            wt = wts.to_wavetable(wavetable, self.wt_length)
            if len(wt) < self.wt_length:
                wt = interpolation._linear(wt, self.wt_length)

            for j, val in enumerate(wt):
                self.wavetables[i][j] = val                

        self.win_length = 0
        for window in windows:
            try:
                self.win_length = max(len(window), self.win_length)
            except TypeError:
                self.win_length = max(DEFAULT_WT_LENGTH, self.win_length)

        self.win_count = len(windows)
        self.windows = np.zeros((self.win_count, self.win_length), dtype='d')

        for i, window in enumerate(windows):
            win = wts.to_window(window, self.win_length)
            if len(wt) < self.wt_length:
                win = interpolation._linear(win, self.win_length)

            for j, val in enumerate(win):
                self.windows[i][j] = val                


    def play(self, length):
        framelength = <int>(length * self.samplerate)
        return self._play(framelength)

    cdef object _play(self, int length):
        cdef int i = 0
        cdef double sample, pulsewidth, freq

        cdef double[:,:] out = np.zeros((length, self.channels), dtype='d')
        cdef double[:] wt_outputs = np.zeros(self.wt_count, dtype='d')
        cdef double[:] win_outputs = np.zeros(self.win_count, dtype='d')

        cdef double isamplerate = 1.0 / self.samplerate
        cdef double ilength = 1.0 / length

        cdef int freq_boundry = max(len(self.freq)-1, 1)
        cdef int amp_boundry = max(len(self.amp)-1, 1)
        cdef int pw_boundry = max(len(self.pulsewidth)-1, 1)
        cdef int wt_mod_boundry = max(len(self.wt_mod)-1, 1)
        cdef int win_mod_boundry = max(len(self.win_mod)-1, 1)

        cdef int wt_boundry = max(self.wt_length-1, 1)
        cdef int win_boundry = max(self.win_length-1, 1)
        cdef int wt_boundry_p = wt_boundry
        cdef int win_boundry_p = win_boundry

        cdef double freq_phase_inc = ilength * freq_boundry
        cdef double amp_phase_inc = ilength * amp_boundry
        cdef double pw_phase_inc = ilength * pw_boundry
        cdef double wt_mod_phase_inc = ilength * wt_mod_boundry
        cdef double win_mod_phase_inc = ilength * win_mod_boundry

        cdef int wi = 0

        for i in range(length):
            freq = interpolation._linear_point(self.freq, self.freq_phase)
            pulsewidth = max(interpolation._linear_point(self.pulsewidth, self.pw_phase), MIN_PULSEWIDTH)
            amp = interpolation._linear_point(self.amp, self.amp_phase)
            self.wt_pos = interpolation._linear_point(self.wt_mod, self.wt_mod_phase)
            self.win_pos = interpolation._linear_point(self.win_mod, self.win_mod_phase)

            ipulsewidth = 1.0/pulsewidth
            wt_boundry_p = <int>max((ipulsewidth * self.wt_length)-1, 1)
            win_boundry_p = <int>max((ipulsewidth * self.win_length)-1, 1)

            for wi in range(self.wt_count):
                wt_outputs[wi] = interpolation._linear_point(self.wavetables[wi], self.wt_phase, pulsewidth)

            for wi in range(self.win_count):
                win_outputs[wi] = interpolation._linear_point(self.windows[wi], self.win_phase, pulsewidth)

            sample = interpolation._linear_pos(wt_outputs, self.wt_pos)
            sample *= interpolation._linear_pos(win_outputs, self.win_pos)
            sample *= amp

            self.freq_phase += freq_phase_inc
            self.amp_phase += amp_phase_inc
            self.pw_phase += pw_phase_inc
            self.wt_mod_phase += wt_mod_phase_inc
            self.win_mod_phase += win_mod_phase_inc
            self.wt_phase += isamplerate * wt_boundry_p * freq
            self.win_phase += isamplerate * win_boundry_p * freq

            while self.wt_phase >= wt_boundry_p:
                self.wt_phase -= wt_boundry_p

            while self.win_phase >= win_boundry_p:
                self.win_phase -= win_boundry_p

            while self.wt_mod_phase >= wt_mod_boundry:
                self.wt_mod_phase -= wt_mod_boundry

            while self.win_mod_phase >= win_mod_boundry:
                self.win_mod_phase -= win_mod_boundry

            while self.pw_phase >= pw_boundry:
                self.pw_phase -= pw_boundry

            while self.amp_phase >= amp_boundry:
                self.amp_phase -= amp_boundry

            while self.freq_phase >= freq_boundry:
                self.freq_phase -= freq_boundry

            for channel in range(self.channels):
                out[i][channel] = sample

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

