#cython: language_level=3

cdef double[:] _wavetable(int, int)
cdef double[:] _window(int, int)
cdef double[:] _adsr(int framelength, int attack, int decay, double sustain, int release)

cdef class Wavetable:
    cdef public double[:] data
    cdef public double lowvalue
    cdef public double highvalue
    cdef public int length

cdef int SINE
cdef int SINEIN 
cdef int SINEOUT
cdef int COS
cdef int TRI
cdef int SAW
cdef int PHASOR
cdef int RSAW
cdef int HANN
cdef int HANNIN
cdef int HANNOUT
cdef int HAMM
cdef int BLACK
cdef int BLACKMAN
cdef int BART
cdef int BARTLETT
cdef int KAISER
cdef int SQUARE
cdef int RND
cdef int LINEAR
cdef int LINE
cdef int TRUNC
cdef int HERMITE
cdef int CONSTANT
cdef int GOGINS

cdef int LEN_WINDOWS
cdef int* ALL_WINDOWS
cdef int LEN_WAVETABLES
cdef int* ALL_WAVETABLES

cpdef double[:] to_window(object w, int wtsize=?)
cpdef double[:] to_wavetable(object w, int wtsize=?)
cpdef list to_lfostack(list lfos, int wtsize=?)
cpdef Wavetable randline(int numpoints, double lowvalue=?, double highvalue=?, int wtsize=?)
cdef double[:] _window(int window_type, int length)
cdef double[:] _adsr(int framelength, int attack, int decay, double sustain, int release)
cpdef double[:] adsr(int length, int attack, int decay, double sustain, int release)
cdef double[:] _wavetable(int wavetable_type, int length)
cpdef double[:] wavetable(int wavetable_type, int length, double[:] data=?)
