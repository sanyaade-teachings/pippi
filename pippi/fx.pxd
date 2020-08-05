#cython: language_level=3

from pippi.wavetables cimport Wavetable
from pippi.soundbuffer cimport SoundBuffer

cdef double[:,:] _norm(double[:,:] snd, double ceiling)
cdef double[:,:] _fir(double[:,:] snd, double[:,:] out, double[:] impulse, bint norm=*)
cpdef SoundBuffer fir(SoundBuffer snd, object impulse, bint normalize=*)
cpdef Wavetable envelope_follower(SoundBuffer snd, double window=*)
cpdef SoundBuffer widen(SoundBuffer snd, object width=*)

cdef class ZenerClipperBL:
    cdef double lastIn
    cdef double _clip(ZenerClipperBL self, double val)
    cdef double _integratedClip(ZenerClipperBL self, double val)
    cdef double _process(ZenerClipperBL self, double val)
    cpdef SoundBuffer process(ZenerClipperBL self, SoundBuffer snd)

cdef class SVF:
    cdef double[4] Az 
    cdef double[2] Bz
    cdef double[2] CLPz
    cdef double[2] CBPz
    cdef double[2] CHPz
    cdef double DLPz
    cdef double DBPz
    cdef double DHPz

    cdef double[2] X

    cdef double lpOut
    cdef double bpOut
    cdef double hpOut

    cdef void _setParams(SVF self, double freq, double res)
    cdef double _process(SVF self, double val)
    cpdef SoundBuffer process(SVF self, SoundBuffer snd, object freq=*, object res=*)

