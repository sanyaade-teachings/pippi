#cython: language_level=3

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

cdef extern from "oscs.pulsar.h":
    ctypedef struct lppulsarosc_t:
        lpfloat_t * wavetables
        size_t wavetable_length
        int num_wavetables
        size_t * wavetable_onsets
        size_t * wavetable_lengths
        lpfloat_t wavetable_morph
        lpfloat_t wavetable_morph_freq

        lpfloat_t * windows
        size_t window_length
        int num_windows
        size_t * window_onsets
        size_t * window_lengths
        lpfloat_t window_morph
        lpfloat_t window_morph_freq

        #bool * burst
        size_t burst_size
        size_t burst_pos 

        lpfloat_t phase
        lpfloat_t saturation
        lpfloat_t pulsewidth
        lpfloat_t samplerate
        lpfloat_t freq


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

cdef extern from "ugens.pulsar.h":
    cdef enum UgenPulsarParams:
        UPULSARIN_WTTABLE,
        UPULSARIN_WTTABLELENGTH,
        UPULSARIN_NUMWTS,
        UPULSARIN_WTOFFSETS,
        UPULSARIN_WTLENGTHS,
        UPULSARIN_WTMORPH,
        UPULSARIN_WTMORPHFREQ,

        UPULSARIN_WINTABLE,
        UPULSARIN_WINTABLELENGTH,
        UPULSARIN_NUMWINS,
        UPULSARIN_WINOFFSETS,
        UPULSARIN_WINLENGTHS,
        UPULSARIN_WINMORPH,
        UPULSARIN_WINMORPHFREQ,

        UPULSARIN_BURSTTABLE,
        UPULSARIN_BURSTSIZE,
        UPULSARIN_BURSTPOS,

        UPULSARIN_PHASE,
        UPULSARIN_SATURATION,
        UPULSARIN_PULSEWIDTH,
        UPULSARIN_SAMPLERATE,
        UPULSARIN_FREQ

    cdef enum UgenPulsarOutputs:
        UPULSAROUT_MAIN,
        UPULSAROUT_WTTABLE,
        UPULSAROUT_WTTABLELENGTH,
        UPULSAROUT_NUMWTS,
        UPULSAROUT_WTOFFSETS,
        UPULSAROUT_WTLENGTHS,
        UPULSAROUT_WTMORPH,
        UPULSAROUT_WTMORPHFREQ,

        UPULSAROUT_WINTABLE,
        UPULSAROUT_WINTABLELENGTH,
        UPULSAROUT_NUMWINS,
        UPULSAROUT_WINOFFSETS,
        UPULSAROUT_WINLENGTHS,
        UPULSAROUT_WINMORPH,
        UPULSAROUT_WINMORPHFREQ,

        UPULSAROUT_BURSTTABLE,
        UPULSAROUT_BURSTSIZE,
        UPULSAROUT_BURSTPOS,

        UPULSAROUT_PHASE,
        UPULSAROUT_SATURATION,
        UPULSAROUT_PULSEWIDTH,
        UPULSAROUT_SAMPLERATE,
        UPULSAROUT_FREQ

    ctypedef struct lpugenpulsar_t:
        lppulsarosc_t * osc
        lpfloat_t outputs[23]

    cdef ugen_t * create_pulsar_ugen()


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
    cdef double next_sample(Graph self)
