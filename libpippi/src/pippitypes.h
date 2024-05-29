/* TYPES */
#ifdef LP_FLOAT
typedef float lpfloat_t;
#else
typedef double lpfloat_t;
#endif

/* Core datatypes */
typedef struct lpbuffer_t {
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
    lpfloat_t data[];
} lpbuffer_t;

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

    uint16_t flags;
    uint16_t type;
    char msg[LPMAXMSG];
    char instrument_name[LPMAXNAME];
} lpmsg_t;

typedef struct lpserialmsg_t {
    uint32_t size; // when greater than 0, the following bytes have a data payload of `size`
    uint16_t type; // corresponds to LPMessageTypes enum from libpippi
    char instrument_name[LPMAXNAME]; // the instrument name, for message routing
} lpserialmsg_t;

typedef struct lparray_t {
    int * data;
    size_t length;
    lpfloat_t phase;
} lparray_t;

typedef struct lppatternbuf_t {
    size_t length;
    unsigned char pattern[LPMAXPAT];
} lppatternbuf_t;


