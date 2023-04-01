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
        int fd
        char * buf
        size_t headsize
        size_t fullsize
        size_t pos
        size_t frames
        int channels

    ctypedef struct lpsampler_t:
        int fd
        char * name
        char * buf
        size_t total_bytes
        size_t length
        int samplerate
        int channels

        size_t length_offset
        size_t samplerate_offset
        size_t channels_offset
        size_t buffer_offset

    ctypedef struct lpmsg_t:
        size_t timestamp
        size_t voice_id
        char instrument_name[LPMAXNAME]
        char msg[LPMAXMSG]


    lpsampler_t * lpsampler_create_from(int bankid, lpbuffer_t * buf)
    lpsampler_t * lpsampler_open(int bankid)
    lpsampler_t * lpsampler_dub(int bankid, lpbuffer_t * buf)
    int lpsampler_close(lpsampler_t *)
    int lpsampler_destroy(lpsampler_t *)
    int lpsampler_get_length(lpsampler_t * sampler, size_t * length)
    int lpsampler_get_samplerate(lpsampler_t * sampler, int * samplerate)
    int lpsampler_get_channels(lpsampler_t * sampler, int * channels)
    lpfloat_t lpsampler_read_sample(lpsampler_t * sampler, size_t frame, int channel)

    lpadcbuf_t * lpadc_create()
    lpadcbuf_t * lpadc_open()
    lpfloat_t lpadc_read_sample(lpadcbuf_t * adcbuf, size_t frame, int channel)
    void lpadc_get_pos(lpadcbuf_t * adcbuf, size_t * pos)
    size_t lpadc_write_sample(lpadcbuf_t * adcbuf, lpfloat_t sample, int channel, ssize_t offset)
    void lpadc_increment_pos(lpadcbuf_t * adcbuf, int count)
    void lpadc_close(lpadcbuf_t * adcbuf)
    void lpadc_destroy(lpadcbuf_t * adcbuf)


cdef class SessionParamBucket:
    cdef object _bus

cdef class ParamBucket:
    cdef object _params
    cdef dict _instrument_params
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
    cdef lpmsg_t * msg
    cdef public int id

cdef class Instrument:
    cdef public str name
    cdef public str path
    cdef public list groups
    cdef public object renderer
    cdef object shutdown
    cdef public object sounds
    cdef public int playing
    cdef public dict params 
    cdef public dict cache
    cdef public size_t last_reload

cdef tuple collect_players(object instrument)
cdef int render_event(object instrument, lpmsg_t * msg)

