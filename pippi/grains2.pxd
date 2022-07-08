cdef extern from "pippicore.h":
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
        lpfloat_t length
        lpfloat_t window_phase_offset
        lpfloat_t pulsewidth
        lptapeosc_t * osc
        lpfloat_t phase
        lpfloat_t pan
        lpfloat_t amp
        lpfloat_t speed
        lpfloat_t skew
        lpbuffer_t * window

    ctypedef struct lpformation_t:
        lpgrain_t ** grains
        size_t numgrains
        size_t maxlength
        size_t minlength
        lpfloat_t spread
        lpfloat_t speed
        lpfloat_t scrub
        lpfloat_t offset
        lpfloat_t skew
        lpfloat_t grainamp
        lpfloat_t pulsewidth 
        lpbuffer_t * window
        lpbuffer_t * current_frame
        lpbuffer_t * rb

    ctypedef struct lpgrain_factory_t:
        lpgrain_t * (*create)(lpfloat_t, lpbuffer_t *, lpbuffer_t *)
        void (*process)(lpgrain_t *, lpbuffer_t *)
        void (*destroy)(lpgrain_t *)

    ctypedef struct lpformation_factory_t:
        lpformation_t * (*create)(int, size_t, size_t, size_t, int, int)
        void (*process)(lpformation_t *)
        void (*destroy)(lpformation_t *)

    extern const lpgrain_factory_t LPGrain
    extern const lpformation_factory_t LPFormation

cdef class Cloud2:
    cdef double[:,:] snd
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

    cdef int[:] mask
    cdef bint has_mask

