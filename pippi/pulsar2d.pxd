#cython: language_level=3

from pippi.interpolation cimport interp_point_t

cdef extern from "pippicore.h":
    ctypedef double lpfloat_t
    ctypedef struct lpbuffer_t:
        size_t length
        int samplerate
        int channels
        lpfloat_t phase
        size_t boundry
        size_t range
        size_t pos
        size_t onset
        int is_looping
        lpfloat_t data[]

    ctypedef struct lparray_t:
        int * data
        size_t length
        lpfloat_t phase

    ctypedef struct lpstack_t:
        lpbuffer_t ** stack
        size_t length
        lpfloat_t phase
        lpfloat_t pos

    ctypedef struct lparray_factory_t:
        lparray_t * (*create)(size_t)
        lparray_t * (*create_from)(int, ...)
        void (*destroy)(lparray_t *)


    ctypedef struct lpbuffer_factory_t:
        lpbuffer_t * (*create)(size_t, int, int)
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

    ctypedef struct lpparam_factory_t:
        lpbuffer_t * (*from_float)(lpfloat_t)
        lpbuffer_t * (*from_int)(int)

    ctypedef struct lpwavetable_factory_t:
        lpbuffer_t * (*create)(int name, size_t length)
        lpstack_t * (*create_stack)(int numtables, ...)
        void (*destroy)(lpbuffer_t*)

    ctypedef struct lpwindow_factory_t:
        lpbuffer_t * (*create)(int name, size_t length)
        lpstack_t * (*create_stack)(int numtables, ...)
        void (*destroy)(lpbuffer_t*)

    cdef extern const lparray_factory_t LPArray
    cdef extern const lpbuffer_factory_t LPBuffer
    cdef extern const lpwavetable_factory_t LPWavetable
    cdef extern const lpwindow_factory_t LPWindow

cdef extern from "oscs.pulsar.h":
    ctypedef struct lppulsarosc_t:
        lpstack_t * wts
        lpstack_t * wins
        lparray_t * burst

        lpfloat_t saturation
        lpfloat_t pulsewidth
        lpfloat_t samplerate
        lpfloat_t freq

    ctypedef struct lppulsarosc_factory_t:
        lppulsarosc_t * (*create)()
        lpfloat_t (*process)(lppulsarosc_t *)
        void (*destroy)(lppulsarosc_t*)

    cdef extern const lppulsarosc_factory_t LPPulsarOsc


cdef class Pulsar2d:
    cdef public double[:] freq
    cdef public double[:] amp
    cdef public double[:] pulsewidth

    cdef public double[:] mask
    cdef public long[:] burst

    cdef double[:,:] wavetables
    cdef double[:,:] windows

    cdef interp_point_t freq_interpolator

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
    cdef double mask_phase
    cdef double burst_phase
    cdef int burst_length

    cdef public int channels
    cdef public int samplerate

    cdef object _play(self, int length)
