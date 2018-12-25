cdef class Pulsar:
    cdef public double[:] freq
    cdef public double[:] amp

    cdef double[:] wavetable

    cdef double[:] window
    cdef public double[:] pulsewidth

    cdef public double phase

    cdef public int channels
    cdef public int samplerate
    cdef public int wtsize

    cdef object _play(self, int length)
