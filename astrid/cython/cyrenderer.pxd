#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer


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

    ctypedef struct lpbuffer_factory_t: 
        lpbuffer_t * (*create)(size_t, int, int)
        void (*copy)(lpbuffer_t *, lpbuffer_t *)
        void (*scale)(lpbuffer_t *, lpfloat_t, lpfloat_t, lpfloat_t, lpfloat_t)
        lpfloat_t (*play)(lpbuffer_t *, lpfloat_t)
        lpbuffer_t * (*mix)(lpbuffer_t *, lpbuffer_t *)
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
        void (*dub)(lpbuffer_t *, lpbuffer_t *)
        void (*env)(lpbuffer_t *, lpbuffer_t *)
        void (*destroy)(lpbuffer_t *)

    extern const lpbuffer_factory_t LPBuffer

cdef extern from "scheduler.h":
    ctypedef struct lpscheduler_t:
        pass

cdef extern from "astrid.h":
    cdef const int LPMAXMSG
    cdef const int LPMAXNAME

    ctypedef struct lpadcbuf_t:
        size_t pos
        char * buf

    ctypedef struct lpmsg_t:
        size_t delay
        size_t voice_id
        char instrument_name[LPMAXNAME]
        char msg[LPMAXMSG]

    int lpadc_create()
    int lpadc_destroy()
    lpadcbuf_t * lpadc_open()
    void lpadc_write_block(lpadcbuf_t * adcbuf, float * block, size_t blocksize_in_bytes)
    lpfloat_t lpadc_read_sample(lpadcbuf_t * adcbuf, size_t pos)



cdef class SessionParamBucket:
    cdef object _bus

cdef class ParamBucket:
    cdef object _params
    cdef str _play_params

cdef class EventContext:
    cdef public dict cache
    cdef public object messages
    cdef public ParamBucket p
    cdef public SessionParamBucket s
    cdef public object client
    cdef public str instrument_name
    cdef public object running
    cdef public object shutdown
    cdef public object stop_me
    cdef public object sounds
    cdef public int count
    cdef public int tick
    cdef public str play_params
    cdef public int id

cdef class Instrument:
    cdef public str name
    cdef public str path
    cdef public object renderer
    cdef public object sounds
    cdef public dict cache
    cdef public size_t last_reload

cdef tuple collect_players(object instrument)
cdef int render_event(object instrument, lpmsg_t * msg)

