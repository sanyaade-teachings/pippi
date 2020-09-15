#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer

cdef class Event:
    cdef public double onset
    cdef public double length
    cdef public double freq
    cdef public double amp
    cdef public double pos
    cdef public int count
    cdef dict _params
    cdef object _before

cdef SoundBuffer render(list events, object callback)
