#cython: language_level=3

cdef class Pulsar2d:
    cdef public double[:] freq
    cdef public double[:] amp
    cdef public double[:] pulsewidth

    cdef double[:,:] wavetables
    cdef double[:,:] windows

    cdef int wt_count
    cdef int wt_length
    cdef double wt_pos
    cdef double wt_phase
    cdef double wt_mod_phase
    cdef double[:] wt_mod

    cdef int win_count
    cdef int win_length
    cdef double win_pos
    cdef double win_phase
    cdef double win_mod_phase
    cdef double[:] win_mod

    cdef double freq_phase
    cdef double pw_phase
    cdef double amp_phase

    cdef public int channels
    cdef public int samplerate

    cdef object _play(self, int length)
