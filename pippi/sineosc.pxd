#cython: language_level=3

cdef class SineOsc:
    cdef public double[:] freq
    cdef public double[:] amp

    cdef double freq_phase
    cdef double amp_phase
    cdef double osc_phase

    cdef public int channels
    cdef public int samplerate
    cdef public int wtsize

    cdef object _play(self, int length)
