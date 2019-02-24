#cython: language_level=3

cdef double[:] _wavetable(int, int)
cdef double[:] _window(int, int)
cdef double[:] _adsr(int framelength, int attack, int decay, double sustain, int release)

cdef class Wavetable:
    cdef public double[:] data
    cdef public double lowvalue
    cdef public double highvalue
    cdef public int length

    cpdef Wavetable clip(Wavetable self, double minval=*, double maxval=*)
    cpdef void drink(Wavetable self, double width=*, object minval=*, object maxval=*, list indexes=*, bint wrap=*)
    cpdef Wavetable harmonics(Wavetable self, object harmonics=*, object weights=*)
    cpdef Wavetable env(Wavetable self, str window_type=*)
    cpdef double max(Wavetable self)
    cpdef void pad(Wavetable self, int numzeros=*)
    cpdef Wavetable padded(Wavetable self, int numzeros=*)
    cpdef void repeat(Wavetable self, int reps=*)
    cpdef Wavetable repeated(Wavetable self, int reps=*)
    cpdef void reverse(Wavetable self)
    cpdef Wavetable reversed(Wavetable self)
    cpdef Wavetable taper(Wavetable self, int length)
    cpdef void skew(Wavetable self, double tip)
    cpdef Wavetable skewed(Wavetable self, double tip)
    cpdef void normalize(Wavetable self, double amount=*)
    cpdef void crush(Wavetable self, int steps)
    cpdef Wavetable crushed(Wavetable self, int steps)
    cpdef double interp(Wavetable self, double pos, str method=*)

cdef double _mag(double[:] data)
cdef double[:] _normalize(double[:] data, double ceiling)

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
cdef int SINC

cdef int LEN_WINDOWS
cdef int* ALL_WINDOWS
cdef int LEN_WAVETABLES
cdef int* ALL_WAVETABLES

cpdef double[:] to_window(object w, int wtsize=?)
cpdef double[:] to_wavetable(object w, int wtsize=?)
cpdef list to_stack(list wavetables, int wtsize=?)
cdef int to_flag(str value)
cpdef Wavetable _randline(int numpoints, double lowvalue=?, double highvalue=?, int wtsize=?)
cdef double[:] _window(int window_type, int length)
cdef double[:] _adsr(int framelength, int attack, int decay, double sustain, int release)
cpdef double[:] adsr(int length, int attack, int decay, double sustain, int release)
cdef double[:] _wavetable(int wavetable_type, int length)
cpdef double[:] wavetable(int wavetable_type, int length, double[:] data=?)
cdef double[:] _seesaw(double[:] wt, int length, double tip=*)
cpdef Wavetable seesaw(object wt, int length, double tip=*)
