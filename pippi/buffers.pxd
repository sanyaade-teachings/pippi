#cython: language_level=3

cdef extern from "pippicore.h":
    ctypedef double lpfloat_t

    ctypedef struct lpbuffer_t:
        lpfloat_t * data
        size_t length
        int samplerate
        int channels

        lpfloat_t phase
        size_t boundry
        size_t pos

    ctypedef struct lpwavetable_factory_t:
        lpbuffer_t * (*create)(const char * name, size_t length)
        void (*destroy)(lpbuffer_t *)

    ctypedef struct lpwindow_factory_t:
        lpbuffer_t * (*create)(const char * name, size_t length)
        void (*destroy)(lpbuffer_t *)

    ctypedef struct lpmemorypool_t:
        unsigned char * pool
        size_t poolsize
        size_t pos

    ctypedef struct lpmemorypool_factory_t:
        unsigned char * pool
        size_t poolsize
        size_t pos

        void (*init)(unsigned char *, size_t)
        lpmemorypool_t * (*custom_init)(unsigned char *, size_t)
        void * (*alloc)(size_t, size_t)
        void * (*custom_alloc)(lpmemorypool_t *, size_t, size_t)
        void (*free)(void *)

    ctypedef struct lpbuffer_factory_t: 
        lpbuffer_t * (*create)(size_t, int, int)
        void (*copy)(lpbuffer_t *, lpbuffer_t *)
        void (*scale)(lpbuffer_t *, lpfloat_t, lpfloat_t, lpfloat_t, lpfloat_t)
        lpfloat_t (*play)(lpbuffer_t *, lpfloat_t)
        lpbuffer_t * (*mix)(lpbuffer_t *, lpbuffer_t *)
        void (*multiply)(lpbuffer_t *, lpbuffer_t *)
        void (*multiply_scalar)(lpbuffer_t *, lpfloat_t)
        int (*buffers_are_equal)(lpbuffer_t *, lpbuffer_t *)
        void (*dub)(lpbuffer_t *, lpbuffer_t *)
        void (*env)(lpbuffer_t *, lpbuffer_t *)
        void (*destroy)(lpbuffer_t *)

    extern const lpbuffer_factory_t LPBuffer
    extern const lpwavetable_factory_t LPWavetable 
    extern const lpwindow_factory_t LPWindow
    extern lpmemorypool_factory_t LPMemoryPool



