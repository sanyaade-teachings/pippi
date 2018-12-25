cdef class Osc:
    cdef public double freq
    cdef public double amp

    cdef double[:] wavetable

    cdef double[:] window
    cdef public double pulsewidth

    cdef double[:] mod
    cdef public double mod_range
    cdef public double mod_freq

    cdef public double phase
    cdef public double win_phase
    cdef public double mod_phase

    cdef public int channels
    cdef public int samplerate
    cdef public int wtsize

    cdef object _play(self, int length)
