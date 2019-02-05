# cython: language_level=3, cdivision=True, wraparound=False, boundscheck=False, initializedcheck=False

from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport Wavetable, to_wavetable, to_window
from pippi cimport interpolation
from pippi.defaults cimport DEFAULT_CHANNELS, DEFAULT_SAMPLERATE
from pippi.fx cimport _norm
from pippi.dsp cimport _mag
import numpy as np
from libc.math cimport signbit

cdef class Waveset:
    def __cinit__(
            Waveset self, 
            object values, 
            int crossings=3, 
            int limit=-1, 
            int modulo=1, 
            int samplerate=-1,
        ):

        self.samplerate = samplerate
        self.limit = limit
        self.modulo = modulo
        self.crossings = max(crossings, 2)

        self.load(values)

    def __getitem__(self, position):
        return self.sets[position]

    def __iter__(self):
        return iter(self.wavesets)

    def __len__(self):
        return len(self.wavesets)

    cpdef void load(Waveset self, object values):
        cdef double[:] raw
        cdef double[:] waveset
        cdef double original_mag = 0
        cdef int waveset_length
        cdef double val, last
        cdef int crossing_count=0, waveset_count=0, waveset_output_count=0, mod_count=0
        cdef int i=1, start=-1, end=-1

        self.wavesets = []
        self.max_length = 0
        self.min_length = 0

        if isinstance(values, SoundBuffer):
            original_mag = _mag(values.frames)
            values = values.remix(1)
            raw = np.ravel(np.array(_norm(values.frames, original_mag), dtype='d'))
            if self.samplerate <= 0:
                self.samplerate = values.samplerate

        elif isinstance(values, Wavetable):
            raw = values.data

        else:
            raw = np.ravel(np.array(values, dtype='d'))

        if self.samplerate <= 0:
            self.samplerate = DEFAULT_SAMPLERATE

        cdef int length = len(raw)

        self.framelength = 0
        last = raw[0]
        start = 0
        mod_count = 0

        while i < length:
            if (signbit(last) and not signbit(raw[i])) or (not signbit(last) and signbit(raw[i])):
                crossing_count += 1

                if crossing_count == self.crossings:
                    waveset_count += 1
                    mod_count += 1
                    crossing_count = 0

                    if mod_count == self.modulo:
                        mod_count = 0
                        waveset_length = i-start
                        waveset = np.zeros(waveset_length, dtype='d')
                        waveset = raw[start:i]
                        self.wavesets += [ waveset ]
                        self.framelength += waveset_length

                        self.max_length = max(self.max_length, waveset_length)

                        if self.min_length == 0:
                            self.min_length = waveset_length
                        else:
                            self.min_length = min(self.min_length, waveset_length)

                        waveset_output_count += 1

                        if self.limit == waveset_output_count:
                            break

                    start = i

            last = raw[i]
            i += 1

    cpdef void up(Waveset self, int factor=2):
        pass

    cpdef void down(Waveset self, int factor=2):
        pass

    cpdef void invert(Waveset self):
        pass

    cpdef SoundBuffer substitute(Waveset self, object waveform):
        cdef double[:] wt = to_wavetable(waveform)
        cdef list out = []
        cdef int i, length
        cdef double maxval
        cdef double[:] replacement

        for i in range(len(self.wavesets)):
            length = len(self.wavesets[i])
            maxval = max(np.abs(self.wavesets[i])) 
            replacement = interpolation._linear(wt, length)

            for j in range(length):
                replacement[j] *= maxval

            out += [ replacement ]

        return self.render(out)

    cpdef Waveset morph(Waveset self, Waveset target):
        return self

    cpdef SoundBuffer render(Waveset self, list wavesets=None, int channels=-1, int samplerate=-1):
        channels = DEFAULT_CHANNELS if channels < 1 else channels
        samplerate = self.samplerate if samplerate < 1 else samplerate

        if wavesets is None:
            wavesets = self.wavesets

        cdef double[:,:] out = np.zeros((self.framelength, channels), dtype='d')        
        cdef int numsets = len(wavesets)
        cdef int i=0, c=0, j=0, oi=0

        for i in range(numsets):
            for j in range(len(wavesets[i])):
                for c in range(channels):
                    out[oi][c] = wavesets[i][j]
                oi += 1

        return SoundBuffer(out, channels=channels, samplerate=samplerate)

    cpdef void normalize(Waveset self, double ceiling=1):
        cdef int i=0, j=0
        cdef double normval = 1
        cdef double maxval = 0 
        cdef int numsets = len(self.wavesets)

        for i in range(numsets):
            maxval = max(np.abs(self.wavesets[i])) 
            normval = ceiling / maxval
            for j in range(len(self.wavesets[i])):
                self.sets[i][j] *= normval

