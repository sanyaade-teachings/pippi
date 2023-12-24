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

    ctypedef struct lpbuffer_factory_t: 
        lpbuffer_t * (*create)(size_t, int, int)

    extern const lpbuffer_factory_t LPBuffer

    ctypedef struct ugen_t:
        void * params
        lpfloat_t (*get_output)(ugen_t * u, int index)
        void (*set_param)(ugen_t * u, int index, void * value)
        void (*process)(ugen_t * u)
        void (*destroy)(ugen_t * u)

cdef extern from "oscs.sine.h":
    ctypedef struct lpsineosc_t:
        lpfloat_t phase
        lpfloat_t freq
        lpfloat_t samplerate
        
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

cdef extern from "ugens.tape.h":
    cdef enum UgenTapeParams:
        UTAPEIN_SPEED,
        UTAPEIN_PHASE,
        UTAPEIN_BUF

    cdef enum UgenTapeOutputs:
        UTAPEOUT_MAIN,
        UTAPEOUT_SPEED,
        UTAPEOUT_PHASE

    ctypedef struct lpugentape_t:
        lptapeosc_t * osc
        lpfloat_t outputs[3]

    cdef ugen_t * create_tape_ugen()


cdef extern from "ugens.utils.h":
    cdef enum UgenUtilsParams:
        UMULTIN_A,
        UMULTIN_B,

    cdef enum UgenUtilsOutputs:
        UMULTOUT_MAIN,
        UMULTOUT_A,
        UMULTOUT_B,

    ctypedef struct lpugenmult_t:
        lpfloat_t a
        lpfloat_t b
        int outputs[3]

    cdef ugen_t * create_mult_ugen()


cdef extern from "ugens.sine.h":
    cdef enum UgenSineParams:
        USINEIN_FREQ,
        USINEIN_PHASE

    cdef enum UgenSineOutputs:
        USINEOUT_MAIN,
        USINEOUT_FREQ,
        USINEOUT_PHASE

    ctypedef struct lpugensine_t:
        lpsineosc_t * osc
        int outputs[3]

    cdef ugen_t * create_sine_ugen()

cdef class Node:
    cdef ugen_t * u
    cdef str ugen_name
    cdef str name
    cdef public object connections
    cdef double mult
    cdef double add

cdef class Graph:
    cdef dict nodes
    cdef object outputs
