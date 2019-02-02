cdef class Osc:
    cdef public double[:] freq
    cdef public double[:] amp
    cdef public double[:] wavetable

    cdef double freq_phase
    cdef double amp_phase
    cdef double wt_phase

    cdef public int channels
    cdef public int samplerate
    cdef public int wtsize

    cdef object _play(self, int length)
