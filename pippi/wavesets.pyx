# cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport Wavetable
import numpy as np

cdef class Waveset:
    def __cinit__(
            Waveset self, 
            object values, 
            int crossings=3, 
            int limit=-1, 
        ):
        
        if isinstance(values, SoundBuffer):
            self.raw = np.ravel(np.array(values.remix(1).frames, dtype='d'))

        elif isinstance(values, Wavetable):
            self.raw = values.data

        else:
            self.raw = np.ravel(np.array(values, dtype='d'))

        self.sets = []
        self.crossings = max(crossings, 2)
        self.max_length = 0
        self.min_length = 0

        cdef double[:] waveset
        cdef int waveset_length
        cdef double val, last=0, crossing_count=0, waveset_count=0
        cdef int i=0, start=-1, end=-1
        cdef int length = len(self.raw)

        while i < length:
            if (last >= 0 and self.raw[i] <= 0) or (last <= 0 and self.raw[i] >= 0):
                if last == 0 and self.raw[i] == 0:
                    continue

                crossing_count += 1

                if start < 0:
                    start = i

                if crossing_count == crossings:
                    waveset_length = i-start
                    waveset = np.zeros(waveset_length, dtype='d')
                    waveset = self.raw[start:i]
                    self.sets += [ waveset ]

                    self.max_length = max(self.max_length, waveset_length)
                    self.min_length = min(self.min_length, waveset_length)

                    crossing_count = 0
                    start = -1
                    waveset_count += 1

                    if limit == waveset_count:
                        break

            last = self.raw[i]
            i += 1


    def __getitem__(self, position):
        return self.sets[position]

    def __iter__(self):
        return iter(self.sets)

    def __len__(self):
        return len(self.sets)

