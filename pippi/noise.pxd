cdef extern from "pippicore.h":
    cdef enum Wavetables:
        WT_SINE,
        WT_COS,
        WT_SQUARE, 
        WT_TRI, 
        WT_SAW,
        WT_RSAW,
        WT_RND,
        WT_USER,
        NUM_WAVETABLES

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

    ctypedef struct lpstack_t:
        lpbuffer_t ** stack
        size_t length
        lpfloat_t phase
        lpfloat_t pos
        int read_index

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


    ctypedef struct lpwavetable_factory_t:
        lpbuffer_t * (*create)(int name, size_t length)
        lpstack_t * (*create_stack)(int numtables, ...)
        void (*destroy)(lpbuffer_t*)

    extern const lpwavetable_factory_t LPWavetable
    extern const lpbuffer_factory_t LPBuffer

cdef extern from "oscs.bln.h":
    ctypedef struct lpblnosc_t:
        lpfloat_t phase
        lpfloat_t phaseinc
        lpfloat_t freq
        lpfloat_t minfreq
        lpfloat_t maxfreq
        lpfloat_t samplerate
        lpbuffer_t * buf
        int gate

    ctypedef struct lpblnosc_factory_t:
        lpblnosc_t * (*create)(lpbuffer_t *, lpfloat_t, lpfloat_t)
        lpfloat_t (*process)(lpblnosc_t *)
        lpbuffer_t * (*render)(lpblnosc_t *, size_t, lpbuffer_t *, int)
        void (*destroy)(lpblnosc_t *)

    extern const lpblnosc_factory_t LPBLNOsc


