#cython: language_level=3

cdef class Alias:
    cdef public double[:] freq
    cdef public double[:] amp

    cdef double freq_phase
    cdef double amp_phase
    cdef int pulse_phase

    cdef public int channels
    cdef public int samplerate

    cdef object _play(self, int length)
