cdef extern from "pippicore.h":
    cdef enum Windows:
        WIN_SINE,
        WIN_SINEIN,
        WIN_SINEOUT,
        WIN_COS,
        WIN_TRI, 
        WIN_PHASOR, 
        WIN_HANN, 
        WIN_HANNIN, 
        WIN_HANNOUT, 
        WIN_RND,
        WIN_SAW,
        WIN_RSAW,
        WIN_USER,
        NUM_WINDOWS

    ctypedef double lpfloat_t

    ctypedef struct lpbuffer_t:
        lpfloat_t * data
        size_t length
        int samplerate
        int channels
        lpfloat_t phase
        size_t boundry
        size_t range
        size_t pos

    ctypedef struct lpstack_t:
        lpbuffer_t ** stack
        size_t length
        lpfloat_t phase
        lpfloat_t pos
        int read_index

    ctypedef struct lpparam_factory_t:
        lpbuffer_t * (*from_float)(lpfloat_t)
        lpbuffer_t * (*from_int)(int)

    ctypedef struct lpbuffer_factory_t:
        lpbuffer_t * (*create)(size_t, int, int)
        lpstack_t * (*create_stack)(int, size_t, int, int)
        void (*copy)(lpbuffer_t *, lpbuffer_t *)
        void (*split2)(lpbuffer_t *, lpbuffer_t *, lpbuffer_t *)
        void (*scale)(lpbuffer_t *, lpfloat_t, lpfloat_t, lpfloat_t, lpfloat_t)
        lpfloat_t (*min)(lpbuffer_t * buf)
        lpfloat_t (*max)(lpbuffer_t * buf)
        lpfloat_t (*mag)(lpbuffer_t * buf)
        lpfloat_t (*play)(lpbuffer_t *, lpfloat_t)
        void (*pan)(lpbuffer_t * buf, lpbuffer_t * pos)
        lpbuffer_t * (*mix)(lpbuffer_t *, lpbuffer_t *)
        lpbuffer_t * (*cut)(lpbuffer_t *, size_t, size_t)
        lpbuffer_t * (*resample)(lpbuffer_t *, size_t)
        void (*multiply)(lpbuffer_t *, lpbuffer_t *)
        void (*multiply_scalar)(lpbuffer_t *, lpfloat_t)
        void (*add)(lpbuffer_t *, lpbuffer_t *)
        void (*add_scalar)(lpbuffer_t *, lpfloat_t)
        void (*subtract)(lpbuffer_t *, lpbuffer_t *)
        void (*subtract_scalar)(lpbuffer_t *, lpfloat_t)
        void (*divide)(lpbuffer_t *, lpbuffer_t *)
        void (*divide_scalar)(lpbuffer_t *, lpfloat_t)
        lpbuffer_t * (*concat)(lpbuffer_t *, lpbuffer_t *)
        int (*buffers_are_equal)(lpbuffer_t *, lpbuffer_t *)
        int (*buffers_are_close)(lpbuffer_t *, lpbuffer_t *, int)
        void (*dub)(lpbuffer_t *, lpbuffer_t *, size_t)
        void (*env)(lpbuffer_t *, lpbuffer_t *)
        void (*destroy)(lpbuffer_t *)
        void (*destroy_stack)(lpstack_t *)

    ctypedef struct lpringbuffer_factory_t:
        lpbuffer_t * (*create)(size_t, int, int)
        void (*fill)(lpbuffer_t *, lpbuffer_t *, int)
        lpbuffer_t * (*read)(lpbuffer_t *, size_t)
        void (*readinto)(lpbuffer_t *, lpfloat_t *, size_t, int)
        void (*writefrom)(lpbuffer_t *, lpfloat_t *, size_t, int)
        void (*write)(lpbuffer_t *, lpbuffer_t *)
        lpfloat_t (*readone)(lpbuffer_t *, int)
        void (*writeone)(lpbuffer_t *, lpfloat_t)
        void (*dub)(lpbuffer_t *, lpbuffer_t *)
        void (*destroy)(lpbuffer_t *)

    extern const lpparam_factory_t LPParam
    extern const lpbuffer_factory_t LPBuffer
    extern const lpringbuffer_factory_t LPRingBuffer


cdef extern from "oscs.tape.h":
    ctypedef struct lptapeosc_t:
        lpfloat_t phase
        lpfloat_t speed
        lpfloat_t pulsewidth
        lpfloat_t samplerate
        lpfloat_t start
        lpfloat_t start_increment
        lpfloat_t range
        lpbuffer_t * buf
        lpbuffer_t * current_frame
        int gate

cdef extern from "microsound.h":
    ctypedef struct lpgrain_t:
        size_t length
        int channels
        lpfloat_t pulsewidth

        size_t range
        size_t start
        size_t offset

        lpfloat_t phase_offset
        lpfloat_t phase
        lpfloat_t pan
        lpfloat_t amp
        lpfloat_t speed
        lpfloat_t skew

        int unused
        int gate

        lpbuffer_t * buf
        lpbuffer_t * window

    ctypedef struct lpformation_t:
        int num_active_grains
        size_t numlayers
        size_t grainlength
        lpfloat_t grainlength_maxjitter
        lpfloat_t grainlength_jitter

        size_t graininterval
        lpfloat_t graininterval_phase
        lpfloat_t graininterval_phase_inc

        lpfloat_t spread
        lpfloat_t speed
        lpfloat_t scrub
        lpfloat_t offset
        lpfloat_t skew
        lpfloat_t amp
        lpfloat_t pan
        lpfloat_t pulsewidth

        lpfloat_t pos
        lpbuffer_t * window
        lpbuffer_t * current_frame
        lpbuffer_t * rb

    ctypedef struct lpformation_factory_t:
        lpformation_t * (*create)(int, int, size_t, size_t, int, int, lpbuffer_t *)
        void (*process)(lpformation_t *)
        void (*destroy)(lpformation_t *)

    extern const lpformation_factory_t LPFormation

cdef class Cloud2:
    cdef unsigned int channels
    cdef unsigned int samplerate

    cdef lpformation_t * formation

    cdef double[:] grainlength
    cdef double[:] grid
    cdef double[:] phase

    """
    cdef double[:] position
    cdef double[:] amp
    cdef double[:] speed
    cdef double[:] spread
    cdef double[:] jitter

    cdef int[:] mask
    cdef bint has_mask
    """

