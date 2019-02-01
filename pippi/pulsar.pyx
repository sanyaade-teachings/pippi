# cython: language_level=3

import numbers
import numpy as np
from pippi.soundbuffer cimport SoundBuffer
from pippi cimport wavetables
from pippi cimport interpolation

cdef inline int DEFAULT_SAMPLERATE = 44100
cdef inline int DEFAULT_CHANNELS = 2
cdef inline double MIN_PULSEWIDTH = 0.001

cdef class Pulsar:
    """ Pulsar synthesis
    """
    def __cinit__(
            self, 
            object wavetable=None, 
            object window=None, 

            object freq=440.0, 
            object pulsewidth=1,
            object amp=1.0, 
            double phase=0, 

            int channels=DEFAULT_CHANNELS,
            int samplerate=DEFAULT_SAMPLERATE,
        ):

        self.freq = wavetables.to_wavetable(freq)
        self.amp = wavetables.to_window(amp)
        self.phase = phase

        self.channels = channels
        self.samplerate = samplerate

        self.pulsewidth = wavetables.to_wavetable(pulsewidth)
        self.wavetable = wavetables.to_wavetable(wavetable)
        self.window = wavetables.to_window(window)

    def play(self, length):
        framelength = <int>(length * self.samplerate)
        return self._play(framelength)

    cdef object _play(self, int length):
        cdef int i = 0
        cdef double sample, pulsewidth, freq

        cdef double wt_phase = 0
        cdef double win_phase = 0
        cdef double freq_phase = 0
        cdef double pw_phase = 0
        cdef double amp_phase = 0

        cdef double[:,:] out = np.zeros((length, self.channels), dtype='d')

        cdef int wtlength = len(self.wavetable)
        cdef int winlength = len(self.window)
        cdef int wtlength_p = wtlength
        cdef int winlength_p = winlength

        cdef double isamplerate = 1.0 / self.samplerate
        cdef double ilength = 1.0 / length

        cdef int freq_boundry = max(len(self.freq)-1, 1)
        cdef int amp_boundry = max(len(self.amp)-1, 1)
        cdef int pw_boundry = max(len(self.pulsewidth)-1, 1)

        cdef int wt_length = len(self.wavetable)
        cdef int wt_boundry = max(wt_length-1, 1)
        cdef int win_length = len(self.window)
        cdef int win_boundry = max(win_length-1, 1)
        cdef int wt_boundry_p = wt_boundry
        cdef int win_boundry_p = win_boundry

        cdef double freq_phase_inc = ilength * freq_boundry
        cdef double amp_phase_inc = ilength * amp_boundry
        cdef double pw_phase_inc = ilength * pw_boundry

        for i in range(length):
            freq = interpolation._linear_point(self.freq, freq_phase)
            pulsewidth = max(interpolation._linear_point(self.pulsewidth, pw_phase), MIN_PULSEWIDTH)
            amp = interpolation._linear_point(self.amp, amp_phase)

            ipulsewidth = 1.0/pulsewidth
            wt_boundry_p = <int>max((ipulsewidth * wt_length)-1, 1)
            win_boundry_p = <int>max((ipulsewidth * win_length)-1, 1)

            freq_phase += freq_phase_inc
            amp_phase += amp_phase_inc
            pw_phase += pw_phase_inc
            wt_phase += isamplerate * wt_boundry_p * freq
            win_phase += isamplerate * win_boundry_p * freq

            while wt_phase >= wt_boundry_p:
                wt_phase -= wt_boundry_p

            while win_phase >= win_boundry_p:
                win_phase -= win_boundry_p

            while pw_phase >= pw_boundry:
                pw_phase -= pw_boundry

            while amp_phase >= amp_boundry:
                amp_phase -= amp_boundry

            while freq_phase >= freq_boundry:
                freq_phase -= freq_boundry

            sample = interpolation._linear_point(self.wavetable, wt_phase, pulsewidth)
            sample *= interpolation._linear_point(self.window, win_phase, pulsewidth)
            sample *= amp

            for channel in range(self.channels):
                out[i][channel] = sample

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

