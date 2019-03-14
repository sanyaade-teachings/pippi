#cython: language_level=3

from pippi.wavetables cimport Wavetable
from pippi.soundbuffer cimport SoundBuffer

cdef double[:,:] _norm(double[:,:] snd, double ceiling)
cdef double[:,:] _fir(double[:,:] snd, double[:,:] out, double[:] impulse, bint norm=*)
cpdef SoundBuffer fir(SoundBuffer snd, object impulse, bint normalize=*)
cpdef Wavetable envelope_follower(SoundBuffer snd, double window=*)
cpdef SoundBuffer widen(SoundBuffer snd, object width=*)
