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

    ctypedef struct ugen_t:
        void * params
        lpfloat_t (*get_output)(ugen_t * u, int index)
        void (*set_param)(ugen_t * u, int index, lpfloat_t value)
        void (*process)(ugen_t * u)
        void (*destroy)(ugen_t * u)

cdef extern from "oscs.sine.h":
    ctypedef struct lpsineosc_t:
        lpfloat_t phase
        lpfloat_t freq
        lpfloat_t samplerate

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
