#cython: language_level=3

cdef extern from "pippicore.h":
    ctypedef double lpfloat_t
    cdef enum Wavetables:
        WT_SINE,
        WT_COS,
        WT_SQUARE, 
        WT_TRI, 
        WT_SAW,
        WT_RSAW,
        WT_RND,
        NUM_WAVETABLES

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
        NUM_WINDOWS

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
        lpbuffer_t * (*create)(int name, size_t length)
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
        lpbuffer_t * (*create_from_float)(lpfloat_t, size_t, int, int)
        #lpstack_t * (*create_stack)(int, size_t, int, int)
        void (*copy)(lpbuffer_t *, lpbuffer_t *)
        void (*clear)(lpbuffer_t *)
        void (*split2)(lpbuffer_t *, lpbuffer_t *, lpbuffer_t *)
        void (*scale)(lpbuffer_t *, lpfloat_t, lpfloat_t, lpfloat_t, lpfloat_t)
        lpfloat_t (*min)(lpbuffer_t * buf)
        lpfloat_t (*max)(lpbuffer_t * buf)
        lpfloat_t (*mag)(lpbuffer_t * buf)
        lpfloat_t (*play)(lpbuffer_t *, lpfloat_t)
        void (*pan)(lpbuffer_t * buf, lpbuffer_t * pos)
        lpbuffer_t * (*mix)(lpbuffer_t *, lpbuffer_t *)
        lpbuffer_t * (*remix)(lpbuffer_t *, int)
        void (*clip)(lpbuffer_t * buf, lpfloat_t minval, lpfloat_t maxval)
        lpbuffer_t * (*cut)(lpbuffer_t * buf, size_t start, size_t length)
        void (*cut_into)(lpbuffer_t * buf, lpbuffer_t * out, size_t start, size_t length)
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
        void (*dub_scalar)(lpbuffer_t *, lpfloat_t, size_t)
        void (*env)(lpbuffer_t *, lpbuffer_t *)
        lpbuffer_t * (*pad)(lpbuffer_t * buf, size_t before, size_t after)
        lpbuffer_t * (*fill)(lpbuffer_t * src, size_t length)
        lpbuffer_t * (*repeat)(lpbuffer_t * src, size_t repeats)
        lpbuffer_t * (*reverse)(lpbuffer_t * buf)
        lpbuffer_t * (*resize)(lpbuffer_t *, size_t)
        void (*plot)(lpbuffer_t * buf)
        void (*destroy)(lpbuffer_t *)
        #void (*destroy_stack)(lpstack_t *)

    ctypedef struct lprand_t:
        lpfloat_t logistic_seed
        lpfloat_t logistic_x

        lpfloat_t lorenz_timestep
        lpfloat_t lorenz_x
        lpfloat_t lorenz_y
        lpfloat_t lorenz_z
        lpfloat_t lorenz_a
        lpfloat_t lorenz_b
        lpfloat_t lorenz_c

        void (*preseed)()
        void (*seed)(int)

        lpfloat_t (*stdlib)(lpfloat_t, lpfloat_t)
        lpfloat_t (*logistic)(lpfloat_t, lpfloat_t)

        lpfloat_t (*lorenz)(lpfloat_t, lpfloat_t)
        lpfloat_t (*lorenzX)(lpfloat_t, lpfloat_t)
        lpfloat_t (*lorenzY)(lpfloat_t, lpfloat_t)
        lpfloat_t (*lorenzZ)(lpfloat_t, lpfloat_t)

        lpfloat_t (*rand_base)(lpfloat_t, lpfloat_t)
        lpfloat_t (*rand)(lpfloat_t, lpfloat_t)
        int (*randint)(int, int)
        int (*randbool)()
        int (*choice)(int)

    extern lprand_t LPRand
    extern const lpbuffer_factory_t LPBuffer
    extern const lpwavetable_factory_t LPWavetable 
    extern const lpwindow_factory_t LPWindow
    extern lpmemorypool_factory_t LPMemoryPool


cdef extern from "spectral.h":
    ctypedef struct lpspectral_factory_t:
        lpbuffer_t * (*convolve)(lpbuffer_t *, lpbuffer_t *)
    extern const lpspectral_factory_t LPSpectral

cdef extern from "soundfile.h":
    ctypedef struct lpsoundfile_factory_t:
        lpbuffer_t * (*read)(const char *)
        void (*write)(const char *, lpbuffer_t *)

    extern const lpsoundfile_factory_t LPSoundFile


