#cython: language_level=3

from pippi.wavetables cimport Wavetable
from pippi.soundbuffer cimport SoundBuffer

cdef double[:,:] _norm(double[:,:] snd, double ceiling)
cdef double[:,:] _fir(double[:,:] snd, double[:,:] out, double[:] impulse, int norm)
cpdef Wavetable envelope_follower(SoundBuffer snd, double window=*)
