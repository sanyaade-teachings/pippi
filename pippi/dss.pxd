#cython: language_level=3

from pippi.breakpoints cimport Breakpoint

cdef class DSS:
    cdef public double[:] freq
    cdef public double[:] xwidth
    cdef public double[:] ywidth
    cdef public double[:] amp
    cdef public double[:] wavetable
    cdef public Breakpoint points

    cdef double freq_phase
    cdef double xwidth_phase
    cdef double ywidth_phase
    cdef double amp_phase
    cdef double wt_phase

    cdef public int channels
    cdef public int samplerate
    cdef public int numpoints
    cdef public int wtsize

    cdef object _play(self, int length)

