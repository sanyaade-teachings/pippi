/* CONSTANTS */
#ifndef PI
#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062
#endif

#ifndef HALFPI
#define HALFPI (PI/2.0)
#endif

#ifndef PI2
#define PI2 (PI*2.0)
#endif

#ifndef EULER
#define EULER 2.718281828459045235360287471352662497757247093
#endif

#define LPVSPEED_MIN 0.001

#define GRID_EMPTY 0x2800
#define GRID_FULL  0x28ff

#define LOGISTIC_SEED_DEFAULT 3.999
#define LOGISTIC_X_DEFAULT 0.555

#define LORENZ_TIMESTEP_DEFAULT 0.011
#define LORENZ_A_DEFAULT 10.0
#define LORENZ_B_DEFAULT 28.0
#define LORENZ_C_DEFAULT (8.0 / 3.0)
#define LORENZ_X_DEFAULT 0.1
#define LORENZ_Y_DEFAULT 0.0
#define LORENZ_Z_DEFAULT 0.0

/* plot width/height is measured in display chars */
#define PLOT_WIDTH 20
#define PLOT_HEIGHT 10

/* braille width/height correspond to the number of dots in 
 * each unicode braille char: 2 columns of 4 dots */
#define BRAILLE_WIDTH 2
#define BRAILLE_HEIGHT 4

/* This is the virtual pixel grid, drawn in braille dot 
 * configurations across the field of braille chars */
#define PIXEL_WIDTH (PLOT_WIDTH * BRAILLE_WIDTH)
#define PIXEL_HEIGHT (PLOT_HEIGHT * BRAILLE_HEIGHT)

#define DEFAULT_CHANNELS 2
#define DEFAULT_SAMPLERATE 48000
#define DEFAULT_TABLESIZE 4096

#ifdef LP_FLOAT
#define HANN_WINDOW_SIZE 256
#else
#define HANN_WINDOW_SIZE 4096
#endif

#ifndef PIPE_BUF
#define PIPE_BUF 4096
#endif

/* Magic number for the FNV-1 hash from:
 * http://www.isthe.com/chongo/src/fnv/fnv.h */
#define FNV1_32_MAGIC_NUMBER ((u_int32_t)0x811c9dc5)

#define LPMAXNAME 16
#define LPMAXMSG (PIPE_BUF - (sizeof(double) * 4) - (sizeof(size_t) * 3) - (sizeof(uint16_t) * 2) - LPMAXNAME)
#define LPMAXPAT 512 - sizeof(size_t)

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

enum LPMessageFlags {
    LPFLAG_NONE            =0,
    LPFLAG_IS_SCHEDULED    =1 << 0,
    LPFLAG_IS_ENCODED_PARAM=1 << 1,
    LPFLAG_IS_FLOAT_ENCODED=1 << 2,
    LPFLAG_IS_INT32_ENCODED=1 << 3,
};

enum LPParamTypes {
    LPPARAM_STRING,
    LPPARAM_INT,
    LPPARAM_SIZE_T,
    LPPARAM_FLOAT,
    LPPARAM_DOUBLE,
    NUM_LPPARAMTYPES,
};

#define PLAY_MESSAGE 'p'
#define UPDATE_MESSAGE 'u'
#define TRIGGER_MESSAGE 't'
#define SERIAL_MESSAGE 'b'
#define SCHEDULE_MESSAGE 's'
#define LOAD_MESSAGE 'l'
#define SHUTDOWN_MESSAGE 'q'
#define SET_COUNTER_MESSAGE 'v'

enum LPMessageTypes {
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
