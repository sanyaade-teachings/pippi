#ifndef LP_CORE_H
#define LP_CORE_H

/* std includes */
#include <assert.h>
#include <locale.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* linux platform includes */
#ifdef __linux__
#include <sys/random.h>
#endif

/* TYPES */
#ifdef LP_FLOAT
typedef float lpfloat_t;
#else
typedef double lpfloat_t;
#endif

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


enum Wavetables {
    WT_SINE,
    WT_COS,
    WT_SQUARE, 
    WT_TRI, 
    WT_SAW,
    WT_RSAW,
    WT_RND,
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
    NUM_WINDOWS
};

enum PanMethods {
    PANMETHOD_CONSTANT,
    PANMETHOD_LINEAR,
    PANMETHOD_SINE,
    PANMETHOD_GOGINS,
    NUM_PANMETHODS
};

#define DEFAULT_CHANNELS 2
#define DEFAULT_SAMPLERATE 48000
#define DEFAULT_TABLESIZE 4096


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

#ifdef LP_FLOAT
#define HANN_WINDOW_SIZE 256
#else
#define HANN_WINDOW_SIZE 4096
#endif
extern const lpfloat_t LPHANN_WINDOW[];

typedef struct lpstack_t {
    lpbuffer_t ** stack;
    size_t length;
    lpfloat_t phase; /* Pos within table */
    lpfloat_t pos; /* Pos within stack */
    int read_index;
} lpstack_t;

typedef struct lparray_t {
    int * data;
    size_t length;
    lpfloat_t phase;
} lparray_t;

/* Users may create custom memorypools. 
 * If the primary memorypool is active, 
 * it will be used to allocate the pool.
 *
 * Otherwise initializtion of the pool 
 * will use the stdlib to calloc the space.
 */
typedef struct lpmemorypool_t {
    unsigned char * pool;
    size_t poolsize;
    size_t pos;
} lpmemorypool_t;

/* Factories & static interfaces */
typedef struct lprand_t {
    lpfloat_t logistic_seed;
    lpfloat_t logistic_x;

    lpfloat_t lorenz_timestep;
    lpfloat_t lorenz_x;
    lpfloat_t lorenz_y;
    lpfloat_t lorenz_z;
    lpfloat_t lorenz_a;
    lpfloat_t lorenz_b;
    lpfloat_t lorenz_c;

    void (*preseed)(void);
    void (*seed)(int);

    lpfloat_t (*stdlib)(lpfloat_t, lpfloat_t);
    lpfloat_t (*logistic)(lpfloat_t, lpfloat_t);

    lpfloat_t (*lorenz)(lpfloat_t, lpfloat_t);
    lpfloat_t (*lorenzX)(lpfloat_t, lpfloat_t);
    lpfloat_t (*lorenzY)(lpfloat_t, lpfloat_t);
    lpfloat_t (*lorenzZ)(lpfloat_t, lpfloat_t);

    lpfloat_t (*rand_base)(lpfloat_t, lpfloat_t);
    lpfloat_t (*rand)(lpfloat_t, lpfloat_t);
    int (*randint)(int, int);
    int (*randbool)(void);
    int (*choice)(int);
} lprand_t;

typedef struct lparray_factory_t {
    lparray_t * (*create)(size_t);
    lparray_t * (*create_from)(int, ...);
    void (*destroy)(lparray_t *);
} lparray_factory_t;

typedef struct lpbuffer_factory_t {
    lpbuffer_t * (*create)(size_t, int, int);
    lpbuffer_t * (*create_from_float)(lpfloat_t value, size_t length, int channels, int samplerate);
    lpbuffer_t * (*create_from_bytes)(char * bytes, size_t length, int channels, int samplerate);
    lpstack_t * (*create_stack)(int, size_t, int, int);
    void (*copy)(lpbuffer_t *, lpbuffer_t *);
    void (*clear)(lpbuffer_t *);
    void (*split2)(lpbuffer_t *, lpbuffer_t *, lpbuffer_t *);
    void (*scale)(lpbuffer_t *, lpfloat_t, lpfloat_t, lpfloat_t, lpfloat_t);
    lpfloat_t (*min)(lpbuffer_t * buf);
    lpfloat_t (*max)(lpbuffer_t * buf);
    lpfloat_t (*mag)(lpbuffer_t * buf);
    lpfloat_t (*play)(lpbuffer_t *, lpfloat_t);
    void (*pan)(lpbuffer_t * buf, lpbuffer_t * pos, int method);
    lpbuffer_t * (*mix)(lpbuffer_t *, lpbuffer_t *);
    lpbuffer_t * (*remix)(lpbuffer_t *, int);
    void (*clip)(lpbuffer_t * buf, lpfloat_t minval, lpfloat_t maxval);
    lpbuffer_t * (*cut)(lpbuffer_t * buf, size_t start, size_t length);
    void (*cut_into)(lpbuffer_t * buf, lpbuffer_t * out, size_t start, size_t length);
    lpbuffer_t * (*resample)(lpbuffer_t *, size_t);
    void (*multiply)(lpbuffer_t *, lpbuffer_t *);
    void (*multiply_scalar)(lpbuffer_t *, lpfloat_t);
    void (*add)(lpbuffer_t *, lpbuffer_t *);
    void (*add_scalar)(lpbuffer_t *, lpfloat_t);
    void (*subtract)(lpbuffer_t *, lpbuffer_t *);
    void (*subtract_scalar)(lpbuffer_t *, lpfloat_t);
    void (*divide)(lpbuffer_t *, lpbuffer_t *);
    void (*divide_scalar)(lpbuffer_t *, lpfloat_t);
    lpbuffer_t * (*concat)(lpbuffer_t *, lpbuffer_t *);
    int (*buffers_are_equal)(lpbuffer_t *, lpbuffer_t *);
    int (*buffers_are_close)(lpbuffer_t *, lpbuffer_t *, int);
    void (*dub)(lpbuffer_t *, lpbuffer_t *, size_t);
    void (*dub_scalar)(lpbuffer_t *, lpfloat_t, size_t);
    void (*env)(lpbuffer_t *, lpbuffer_t *);
    lpbuffer_t * (*pad)(lpbuffer_t * buf, size_t before, size_t after);
    void (*taper)(lpbuffer_t * buf, size_t start, size_t end);
    lpbuffer_t * (*trim)(lpbuffer_t * buf, size_t start, size_t end, lpfloat_t threshold, int window);
    lpbuffer_t * (*fill)(lpbuffer_t * src, size_t length);
    lpbuffer_t * (*repeat)(lpbuffer_t * src, size_t repeats);
    lpbuffer_t * (*reverse)(lpbuffer_t * buf);
    lpbuffer_t * (*resize)(lpbuffer_t *, size_t);
    void (*plot)(lpbuffer_t * buf);
    void (*destroy)(lpbuffer_t *);
    void (*destroy_stack)(lpstack_t *);
} lpbuffer_factory_t;

typedef struct lpringbuffer_factory_t {
    lpbuffer_t * (*create)(size_t, int, int);
    void (*fill)(lpbuffer_t *, lpbuffer_t *, int);
    lpbuffer_t * (*read)(lpbuffer_t *, size_t);
    void (*readinto)(lpbuffer_t *, lpfloat_t *, size_t, int);
    void (*writefrom)(lpbuffer_t *, lpfloat_t *, size_t, int);
    void (*write)(lpbuffer_t *, lpbuffer_t *);
    lpfloat_t (*readone)(lpbuffer_t *, int);
    void (*writeone)(lpbuffer_t *, lpfloat_t);
    void (*dub)(lpbuffer_t *, lpbuffer_t *);
    void (*destroy)(lpbuffer_t *);
} lpringbuffer_factory_t;

typedef struct lpparam_factory_t {
    lpbuffer_t * (*from_float)(lpfloat_t);
    lpbuffer_t * (*from_int)(int);
} lpparam_factory_t;

typedef struct lpmemorypool_factory_t {
    /* This is the primary memorypool. */
    unsigned char * pool;
    size_t poolsize;
    size_t pos;

    void (*init)(unsigned char *, size_t);
    lpmemorypool_t * (*custom_init)(unsigned char *, size_t);
    void * (*alloc)(size_t, size_t);
    void * (*custom_alloc)(lpmemorypool_t *, size_t, size_t);
    void (*free)(void *);
} lpmemorypool_factory_t;

typedef struct lpinterpolation_factory_t {
    lpfloat_t (*linear_pos)(lpbuffer_t *, lpfloat_t);
    lpfloat_t (*linear)(lpbuffer_t *, lpfloat_t);
    lpfloat_t (*linear_channel)(lpbuffer_t *, lpfloat_t, int);
    lpfloat_t (*hermite_pos)(lpbuffer_t *, lpfloat_t);
    lpfloat_t (*hermite)(lpbuffer_t *, lpfloat_t);
} lpinterpolation_factory_t;

typedef struct lpwavetable_factory_t {
    lpbuffer_t * (*create)(int name, size_t length);
    lpstack_t * (*create_stack)(int numtables, ...);
    void (*destroy)(lpbuffer_t*);
} lpwavetable_factory_t;

typedef struct lpwindow_factory_t {
    lpbuffer_t * (*create)(int name, size_t length);
    lpstack_t * (*create_stack)(int numtables, ...);
    void (*destroy)(lpbuffer_t*);
} lpwindow_factory_t;

typedef struct lpfx_factory_t {
    lpfloat_t (*read_skewed_buffer)(lpfloat_t freq, lpbuffer_t * buf, lpfloat_t phase, lpfloat_t skew);
    lpfloat_t (*lpf1)(lpfloat_t x, lpfloat_t * y, lpfloat_t cutoff, lpfloat_t samplerate);
    void (*convolve)(lpbuffer_t * a, lpbuffer_t * b, lpbuffer_t * out);
    void (*norm)(lpbuffer_t * buf, lpfloat_t ceiling);
    lpfloat_t (*crush)(lpfloat_t val, int bits);
} lpfx_factory_t;

/* Interfaces */
extern const lparray_factory_t LPArray;
extern const lpbuffer_factory_t LPBuffer;
extern const lpringbuffer_factory_t LPRingBuffer;

extern const lpwavetable_factory_t LPWavetable;
extern const lpwindow_factory_t LPWindow;
extern const lpfx_factory_t LPFX;

extern lprand_t LPRand;
extern const lpparam_factory_t LPParam;
extern lpmemorypool_factory_t LPMemoryPool;
extern const lpinterpolation_factory_t LPInterpolation;

/* Utilities */

/* The zapgremlins() routine was written by James McCartney as part of SuperCollider:
 * https://github.com/supercollider/supercollider/blob/f0d4f47a33b57b1f855fe9ca2d4cb427038974f0/headers/plugin_interface/SC_InlineUnaryOp.h#L35
 *
 * SuperCollider real time audio synthesis system
 * Copyright (c) 2002 James McCartney. All rights reserved.
 * http://www.audiosynth.com
 *
 * He says:
 *      This is a function for preventing pathological math operations in ugens.
 *      It can be used at the end of a block to fix any recirculating filter values.
 */
lpfloat_t lpzapgremlins(lpfloat_t x);

/* These are little value scaling helper routines. */
/* lpwv wraps a given value between min and max through probably wrong basic arithmetic */
lpfloat_t lpwv(lpfloat_t value, lpfloat_t min, lpfloat_t max);
/* lpsv scales a value normally 0-1 to a custom min and max range */
lpfloat_t lpsv(lpfloat_t value, lpfloat_t min, lpfloat_t max);
/* lpsvf does the same but allows a custom from and to range (like from -1 to 1) */
lpfloat_t lpsvf(lpfloat_t value, lpfloat_t min, lpfloat_t max, lpfloat_t from, lpfloat_t to);

lpfloat_t lpfmax(lpfloat_t a, lpfloat_t b);
lpfloat_t lpfmin(lpfloat_t a, lpfloat_t b);
lpfloat_t lpfabs(lpfloat_t value);
lpfloat_t lpfpow(lpfloat_t value, int exp);

#endif
