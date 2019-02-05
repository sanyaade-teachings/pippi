# cython: language_level=3, cdivision=True, wraparound=False, boundscheck=False, initializedcheck=False

from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport Wavetable
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

        cdef double original_mag = 0
        
        if isinstance(values, SoundBuffer):
            original_mag = _mag(values.frames)
            values = values.remix(1)
            self.raw = np.ravel(np.array(_norm(values.frames, original_mag), dtype='d'))
            if self.samplerate <= 0:
                self.samplerate = values.samplerate

        elif isinstance(values, Wavetable):
            self.raw = values.data

        else:
            self.raw = np.ravel(np.array(values, dtype='d'))

        if self.samplerate <= 0:
            self.samplerate = DEFAULT_SAMPLERATE

        self.sets = []
        self.crossings = max(crossings, 2)
        self.max_length = 0
        self.min_length = 0

        self.load()

    def __getitem__(self, position):
        return self.sets[position]

    def __iter__(self):
        return iter(self.sets)

    def __len__(self):
        return len(self.sets)

    cpdef void load(Waveset self):
        cdef double[:] waveset
        cdef int waveset_length
        cdef double val, last
        cdef int crossing_count=0, waveset_count=0, waveset_output_count=0, mod_count=0
        cdef int i=1, start=-1, end=-1
        cdef int length = len(self.raw)
        self.framelength = 0

        last = self.raw[0]
        start = 0
        mod_count = 0
        while i < length:
            if (signbit(last) and not signbit(self.raw[i])) or (not signbit(last) and signbit(self.raw[i])):
                crossing_count += 1

                if start < 0:
                    start = i

                if crossing_count == self.crossings:
                    waveset_count += 1
                    mod_count += 1
                    crossing_count = 0

                    if mod_count == self.modulo:
                        mod_count = 0
                        waveset_length = i-start
                        waveset = np.zeros(waveset_length, dtype='d')
                        waveset = self.raw[start:i]
                        self.sets += [ waveset ]
                        self.framelength += waveset_length

                        self.max_length = max(self.max_length, waveset_length)
                        self.min_length = min(self.min_length, waveset_length)

                        waveset_output_count += 1

                        if self.limit == waveset_output_count:
                            break

                    start = i

            last = self.raw[i]
            i += 1

    cpdef void up(Waveset self, int factor=2):
        pass

    cpdef void down(Waveset self, int factor=2):
        pass

    cpdef void invert(Waveset self):
        pass

    cpdef void substitute(Waveset self, Wavetable waveform):
        pass

    cpdef Waveset morph(Waveset self, Waveset target):
        return self

    cpdef SoundBuffer render(Waveset self, int channels=-1, int samplerate=-1):
        channels = DEFAULT_CHANNELS if channels < 1 else channels
        samplerate = self.samplerate if samplerate < 1 else samplerate

        cdef double[:,:] out = np.zeros((self.framelength, channels), dtype='d')        
        cdef int numsets = len(self.sets)
        cdef int i=0, c=0, j=0, oi=0

        for i in range(numsets):
            for j in range(len(self.sets[i])):
                for c in range(channels):
                    out[oi][c] = self.sets[i][j]
                oi += 1

        return SoundBuffer(out, channels=channels, samplerate=samplerate)

    cpdef void normalize(Waveset self, double ceiling=1):
        cdef int i=0, j=0
        cdef double normval = 1
        cdef double maxval = 0 
        cdef int numsets = len(self.sets)

        for i in range(numsets):
            maxval = max(np.abs(self.sets[i])) 
            normval = ceiling / maxval
            for j in range(len(self.sets[i])):
                self.sets[i][j] *= normval

