#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer

cdef class Sampler:
    cdef list snds
    cdef dict sndmap
    cpdef SoundBuffer play(Sampler self, double freq=*, object length=*)
    cpdef tuple getsnd(Sampler self, double freq)
