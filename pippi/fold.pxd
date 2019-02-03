#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer

cdef class Fold:
    cdef public double[:] wavetable
    cdef public double[:] factors
    cdef public double factFreq
    cdef public double freq
    cdef public double amp
    cdef int channels
    cdef int samplerate
    cdef int wtsize
    cdef SoundBuffer _play(Fold self, double length)
