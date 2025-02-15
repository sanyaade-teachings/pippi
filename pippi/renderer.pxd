#cython: language_level=3

from libc.stdint cimport uint16_t, uint32_t, int32_t
from pippi.soundbuffer cimport SoundBuffer

cdef extern from "stdint.h":
    ctypedef uint32_t u_int32_t

cdef extern from "pippicore.h":
    ctypedef double lpfloat_t

    lpfloat_t lpzapgremlins(lpfloat_t x);
    lpfloat_t lpfilternan(lpfloat_t x);
    u_int32_t lphashstr(char * str)

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

cdef extern from "astrid.h":
    cdef const int LPMAXMSG
    cdef const int LPMAXNAME
    cdef const int NOTE_ON
    cdef const int NOTE_OFF
    cdef const int CONTROL_CHANGE
    cdef const int ASTRID_SAMPLERATE
    cdef const int ASTRID_CHANNELS
    cdef const int NAME_MAX

    ctypedef struct lpscheduler_t:
        pass

    cdef enum LPMessageFlags:
        LPFLAG_NONE,
        LPFLAG_IS_SCHEDULED,
        NUM_LPMESSAGEFLAGS

    cdef enum LPMessageTypes:
        LPMSG_EMPTY,
        LPMSG_PLAY,
        LPMSG_TRIGGER,
        LPMSG_UPDATE,
        LPMSG_SERIAL,
        LPMSG_SCHEDULE,
        LPMSG_LOAD,
        LPMSG_RENDER_COMPLETE,
        LPMSG_DATA,
        LPMSG_SHUTDOWN,
        LPMSG_SET_COUNTER,
        LPMSG_MIDI_FROM_DEVICE,
        LPMSG_MIDI_TO_DEVICE,
        NUM_LPMESSAGETYPES

    ctypedef struct lpmsg_t:
        double initiated
        double scheduled 
        double completed
        double max_processing_time
        size_t onset_delay
        size_t voice_id
        size_t count
        uint16_t flags
        uint16_t type
        char msg[LPMAXMSG]
        char instrument_name[LPMAXNAME]

    ctypedef struct lpmidievent_t:
        double onset
        double now
        double length
        char type
        char note
        char velocity
        char program
        char bank_msb
        char bank_lsb
        char channel

    ctypedef struct lpinstrument_t:
        const char * name
        int channels
        volatile int is_running
        volatile int is_waiting
        int has_been_initialized
        lpfloat_t samplerate

        char adcname[4096]
        lpbuffer_t * adcbuf
        char resamplername[4096]
        lpbuffer_t * resamplerbuf

        char qname[NAME_MAX]
        char external_relay_name[NAME_MAX]
        int msgq
        int exmsgq
        lpmsg_t msg
        lpmsg_t cmd

        lpscheduler_t * async_mixer

    lpbuffer_t * lpsampler_aquire_and_map(char * name);
    int lpsampler_release_and_unmap(char * name, lpbuffer_t * buf);
    int lpsampler_aquire(char * name);
    int lpsampler_release(char * name);
    int lpsampler_read_ringbuffer_block(char * name, lpbuffer_t * buf, size_t offset_in_frames, lpbuffer_t * out);
    int lpsampler_write_ringbuffer_block(char * name, lpbuffer_t * buf, float ** block, int channels, size_t blocksize_in_frames);
    lpbuffer_t * lpsampler_create(char * name, double length_in_seconds, int channels, int samplerate);
    int lpsampler_destroy(char * name);
    int lpsampler_destroy_and_unmap(char * name, lpbuffer_t * buf);

    int lpipc_getid(char * path)
    ssize_t astrid_get_voice_id()

    int send_message(char * qname, lpmsg_t msg)

    int midi_triggerq_open()
    int midi_triggerq_schedule(int qfd, lpmidievent_t t)
    int midi_triggerq_close(int qfd)

    int astrid_msgq_open(char * qname)
    int astrid_msgq_read(int mqd, lpmsg_t * msg)
    int astrid_msgq_close(int mqd)

    int lpmidi_setcc(int device_id, int cc, int value)
    int lpmidi_getcc(int device_id, int cc)
    int lpmidi_setnote(int device_id, int note, int velocity)
    int lpmidi_getnote(int device_id, int note)
    int lpmidi_relay_to_instrument(char * instrument_name, unsigned char mtype, unsigned char mid, unsigned char mval)

    int lpserial_getctl(int device_id, int ctl, lpfloat_t * value)

    int32_t astrid_instrument_get_param_int32(lpinstrument_t * instrument, int param_index, int32_t default_value)
    void astrid_instrument_set_param_int32(lpinstrument_t * instrument, int param_index, int32_t value)
    #void astrid_instrument_set_param_patternbuf(lpinstrument_t * instrument, int param_index, lppatternbuf_t * patternbuf)
    #lppatternbuf_t astrid_instrument_get_param_patternbuf(lpinstrument_t * instrument, int param_index)
    void astrid_instrument_set_param_float(lpinstrument_t * instrument, int param_index, lpfloat_t value)
    lpfloat_t astrid_instrument_get_param_float(lpinstrument_t * instrument, int param_index, lpfloat_t default_value)
    void astrid_instrument_set_param_float_list(lpinstrument_t * instrument, int param_index, lpfloat_t * value, size_t size)
    void astrid_instrument_get_param_float_list(lpinstrument_t * instrument, int param_index, size_t size, lpfloat_t * list)
    lpfloat_t astrid_instrument_get_param_float_list_item(lpinstrument_t * instrument, int param_index, size_t size, int item_index, lpfloat_t default_value)

    int astrid_instrument_restore_param_session_snapshot(lpinstrument_t * instrument, int snapshot_id)
    int astrid_instrument_save_param_session_snapshot(lpinstrument_t * instrument, int num_params, int snapshot_id)

    void scheduler_schedule_event(lpscheduler_t * s, lpbuffer_t * buf, size_t delay)
    int lpscheduler_get_now_seconds(double * now)

    lpbuffer_t * deserialize_buffer(char * str, lpmsg_t * msg)
    int astrid_instrument_publish_bufstr(char * instrument_name, unsigned char * bufstr, size_t size)

    lpinstrument_t * astrid_instrument_start(
        const char * name, 
        int channels, 
        int ext_relay_enabled,
        double adc_length,
        double resampler_length,
        void * ctx, 
        char * tty,
        char * midi_device_name,
        int (*stream)(size_t blocksize, float ** input, float ** output, void * instrument),
        int (*renderer)(void * instrument),
        int (*update)(void * instrument),
        int (*trigger)(void * instrument)
    )

    int astrid_instrument_stop(lpinstrument_t * instrument)
    int astrid_instrument_tick(lpinstrument_t * instrument)
    void scheduler_cleanup_nursery(lpscheduler_t * s)
    int relay_message_to_seq(lpinstrument_t * instrument)
    int send_message(char * qname, lpmsg_t msg)
    int send_play_message(lpmsg_t msg)
    int send_serial_message(lpmsg_t msg)

cdef class MessageEvent:
    cdef lpmsg_t * msg
    cpdef int schedule(MessageEvent self, double now=*)

cdef class MidiEvent:
    cdef lpmidievent_t * event
    cpdef int schedule(MidiEvent self, double now)

cdef class ParamBucket:
    cdef object _params
    cdef str _play_params

cdef class EventTriggerFactory:
    cpdef midi(self, double onset, double length, double freq=*, double amp=*, int note=*, int program=*, int bank_msb=*, int bank_lsb=*, int channel=*, int device=*)

cdef class MidiEventListenerProxy:
    cdef int default_device_id
    cpdef float cc(self, int cc, int device_id=*)
    cpdef int cci(self, int cc, int device_id=*)
    cpdef float note(self, int note, int device_id=*)
    cpdef int notei(self, int note, int device_id=*)

cdef class SerialEventListenerProxy:
    cpdef lpfloat_t ctl(self, int ctl, int device_id=*)

cdef class Instrument:
    cdef public str name
    cdef public str path
    cdef public object renderer
    cdef public object graph
    cdef public dict instrument_param_type_map
    cdef public dict instrument_param_hash_map
    cdef public dict cache
    cdef public lpmsg_t msg # a copy of the last message received
    cdef public size_t last_reload
    cdef public double max_processing_time
    cdef public int channels
    cdef public double samplerate
    cdef public int default_midi_device
    cdef char * ascii_name # instrument name as a c string
    cdef char * midi_device_name
    cdef lpinstrument_t * i
    cpdef EventContext get_event_context(Instrument self, str msgstr=*, bint with_graph=*)
    cpdef lpmsg_t get_message(Instrument self)
    cdef SoundBuffer read_from_adc(Instrument self, double length, double offset=*, int channels=*, int samplerate=*)
    cdef SoundBuffer read_from_resampler(Instrument self, double length, double offset=*, int channels=*, int samplerate=*, str instrument=*)
    cdef SoundBuffer read_block_from_sampler(Instrument self, str name, double length, double offset=*, int channels=*, int samplerate=*)
    cdef SoundBuffer read_from_sampler(Instrument self, str name)
    cdef void save_to_sampler(Instrument self, str name, SoundBuffer snd)

cdef class SessionParamBucket:
    cdef Instrument instrument

cdef class EventContext:
    cdef public dict cache
    cdef public ParamBucket p
    cdef public SessionParamBucket s
    cdef public EventTriggerFactory t
    cdef public MidiEventListenerProxy m
    cdef public SerialEventListenerProxy b
    cdef public str instrument_name
    cdef public object sounds
    cdef public object graph
    cdef public int count
    cdef public int tick
    cdef public int vid
    cdef public double now
    cdef public double max_processing_time
    cdef public Instrument instrument

cdef tuple collect_players(Instrument instrument)
cdef int render_event(Instrument instrument, str msgstr)
cdef set collect_trigger_planners(Instrument instrument)
cdef int trigger_events(Instrument instrument)

