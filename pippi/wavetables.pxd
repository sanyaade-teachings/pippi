cdef double[:] _wavetable(int, int)
cdef double[:] _window(int, int)
cdef double[:] _adsr(int framelength, int attack, int decay, double sustain, int release)

cdef class Wavetable:
    cdef public double[:] data

cdef int SINE
cdef int COS
cdef int TRI
cdef int SAW
cdef int PHASOR
cdef int RSAW
cdef int HANN
cdef int HAMM
cdef int BLACK
cdef int BLACKMAN
cdef int BART
cdef int BARTLETT
cdef int KAISER
cdef int SQUARE
cdef int RND

cdef int LEN_WINDOWS
cdef int *ALL_WINDOWS
cdef int LEN_WAVETABLES
cdef int *ALL_WAVETABLES

