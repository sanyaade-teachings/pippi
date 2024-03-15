/* TYPES */
#ifdef LP_FLOAT
typedef float lpfloat_t;
#else
typedef double lpfloat_t;
#endif

/* Core datatypes */
typedef struct lpbuffer_t {
    lpfloat_t * data;
    size_t length;
    int samplerate;
    int channels;

    /* used for different types of playback */
    lpfloat_t phase;
    size_t boundry;
    size_t range;
    size_t pos;
    size_t onset;
    int is_looping;
} lpbuffer_t;

extern const lpfloat_t LPHANN_WINDOW[];

// Used for messaging between astrid instruments,
// but included in pippicore for embedded use and 
// external messaging support.
typedef struct lpmsg_t {
    /* Timestamp when the message was initiated. 
     *
     * This is assigned at the same time that the 
     * voice ID is assigned to the message sequence.
     * */
    double initiated;     

    /* Relative delay for target completion time 
     * from initiation time.
     *
     * This value is given by the score as the onset time
     * in seconds and is relative to the initiation time.
     * */
    double scheduled;     

    /* Timestamp of render completion or trigger away.
     *
     * This is assigned either at the point that the message 
     * is deserialized along with the buffer and placed onto 
     * the mixer queue, or just after a MIDI event or other 
     * trigger has been sent away.
     *
     * Donno if it is needed, since the message will be 
     * destroyed just after it is set, ideally? Or will it be?
     *
     * Possibly better just to log this at the debug level.
     * */
    double completed;     

    /* The longest a message in this sequence has taken 
     * so far to be processed and reach completion.
     *
     * This is used by the seq scheduler to estimate how
     * far ahead of the target time the message should be
     * sent to the renderer.
     *
     * The initial value before any estimates can be made 
     * is 75% of the scheduled interval time.
     *
     * The estimated value is overwritten each time the 
     * completed timestamp is recorded, by taking the difference 
     * between the initiation time and the completed time.
     *
     * It is preserved... where, in the instrument cache? sqlite?
     * */
    double max_processing_time;     

    /* Time of arrival at mixer minus scheduled target
     * time rounded to nearest frame */
    size_t onset_delay;   

    /* The voice ID is also a message sequence ID */
    size_t voice_id;
    size_t count;

    uint16_t type;
    char msg[LPMAXMSG];
    char instrument_name[LPMAXNAME];
} lpmsg_t;

enum Wavetables {
    WT_SINE,
    WT_COS,
    WT_SQUARE, 
    WT_TRI, 
    WT_TRI2, 
    WT_SAW,
    WT_RSAW,
    WT_RND,
    WT_USER,
    NUM_WAVETABLES
};

enum Windows {
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
    WIN_USER,
    NUM_WINDOWS
};

enum PanMethods {
    PANMETHOD_CONSTANT,
    PANMETHOD_LINEAR,
    PANMETHOD_SINE,
    PANMETHOD_GOGINS,
    NUM_PANMETHODS
};

enum LPMessageTypes {
    LPMSG_EMPTY,
    LPMSG_PLAY,
    LPMSG_TRIGGER,
    LPMSG_UPDATE,
    LPMSG_SERIAL,
    LPMSG_SCHEDULE,
    LPMSG_LOAD,
    LPMSG_RENDER_COMPLETE,
    LPMSG_SHUTDOWN,
    LPMSG_SET_COUNTER,
    NUM_LPMESSAGETYPES
};

// These serial message wrappers are experimental
enum LPSerialParamTypes {
    LPSERIAL_PARAM_BOOL,    /*  0 or 1 */
    LPSERIAL_PARAM_CTL,     /*  0 to 1 */
    LPSERIAL_PARAM_SIG,     /* -1 to 1 */
    LPSERIAL_PARAM_CHAR,    /* unsigned char */
    LPSERIAL_PARAM_INT,     /* signed int */
    LPSERIAL_PARAM_SIZE,    /* ssize_t */
    LPSERIAL_PARAM_UINT,    /* unsigned int */
    LPSERIAL_PARAM_USIZE,   /* size_t */
    LPSERIAL_PARAM_DOUBLE,  /* double */
    LPSERIAL_PARAM_FLOAT,   /* float */

    /* TODO add support for these */
    LPSERIAL_PARAM_MIDI,    /* midi bytes */
    LPSERIAL_PARAM_PCM,     /* PCM audio */
    LPSERIAL_PARAM_SHUTDOWN,
    NUM_LPSERIAL_PARAMS
};

typedef struct lpserialevent_t {
    double onset;
    double now;
    double length;
    lpmsg_t msg;
    int group;
    int device;
} lpserialevent_t;

typedef struct lparray_t {
    int * data;
    size_t length;
    lpfloat_t phase;
} lparray_t;
