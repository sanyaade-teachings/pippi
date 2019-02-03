#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer

cdef class DelayLine:
    cdef short[:] buf
    cdef int position

cdef class Pluck:
    cdef DelayLine upper_rail
    cdef DelayLine lower_rail

    cdef double amp
    cdef double freq
    cdef double pick
    cdef double pickup

    cdef short state
    cdef int pickup_location
    cdef int rail_length
    cdef public double[:] seed

    cdef int samplerate
    cdef int channels


    cpdef short get_sample(Pluck self, DelayLine dline, int position)
    cpdef double next_sample(Pluck self)
    cpdef SoundBuffer play(Pluck self, double length, object seed=*)
