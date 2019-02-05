#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport Wavetable

cdef class Waveset:
    cdef public list sets
    cdef public int crossings
    cdef public int max_length
    cdef public int min_length
    cdef public int framelength
    cdef public int samplerate
    cdef public int limit
    cdef public int modulo

    cpdef void load(Waveset self, object values)
    cpdef void normalize(Waveset self, double ceiling=*)
    cpdef void up(Waveset self, int factor=*)
    cpdef void down(Waveset self, int factor=*)
    cpdef void invert(Waveset self)
    cpdef void substitute(Waveset self, Wavetable waveform)
    cpdef Waveset morph(Waveset self, Waveset target)
    cpdef SoundBuffer render(Waveset self, int channels=*, int samplerate=*)
