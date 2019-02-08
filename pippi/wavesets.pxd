#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport Wavetable

cdef class Waveset:
    cdef public list wavesets
    cdef public int crossings
    cdef public int max_length
    cdef public int min_length
    cdef public int samplerate
    cdef public int limit
    cdef public int modulo

    cpdef void load(Waveset self, object values)
    cpdef void normalize(Waveset self, double ceiling=*)
    cpdef void up(Waveset self, int factor=*)
    cpdef void down(Waveset self, int factor=*)
    cpdef void invert(Waveset self)
    cdef void _slice(Waveset self, double[:] raw, int start, int end)
    cpdef SoundBuffer substitute(Waveset self, object waveform)
    cpdef SoundBuffer harmonic(Waveset self, list harmonics=*, list weights=*)
    cpdef SoundBuffer morph(Waveset self, Waveset target, object curve=*)
    cpdef SoundBuffer render(Waveset self, list wavesets=*, int channels=*, int samplerate=*)
