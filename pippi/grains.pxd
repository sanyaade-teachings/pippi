#from pippi.ugens.core cimport ugen_t

cdef class Cloud:
    cdef float** snd
    cdef unsigned int framelength
    cdef double length
    cdef unsigned int channels
    cdef unsigned int samplerate

    cdef unsigned int wtsize

    cdef double[:] window
    cdef double[:] position
    cdef double[:] amp
    cdef double[:] speed
    cdef double[:] spread
    cdef double[:] jitter
    cdef double[:] grainlength
    cdef double[:] grid

    cdef double[:] lpf
    cdef bint has_lpf

    cdef double[:] hpf
    cdef bint has_hpf

    cdef double[:] bpf
    cdef bint has_bpf

    cdef int[:] mask
    cdef bint has_mask

