#ifndef LP_MIR_H
#define LP_MIR_H

#include "pippicore.h"

typedef struct lpyin_t {
    lpbuffer_t * block;
    int samplerate;
    int blocksize;
    int stepsize; /* overlap between analysis blocks */
    lpfloat_t last_pitch;
    lpfloat_t threshold;
    lpfloat_t fallback;
    int offset;
    int elapsed;

    lpbuffer_t * tmp; /* Temp buffer for processing */

    int tau_max;
    int tau_min;
} lpyin_t;

/**
 * Coyote onset detector ported with permission from 
 * the SuperCollider BatUGens by Batuhan Bozkurt.
 * https://github.com/earslap/SCPlugins
 */
typedef struct lpcoyote_t {
    lpfloat_t track_fall_time;
    lpfloat_t slow_lag_time, fast_lag_time, fast_lag_mul;
    lpfloat_t thresh, min_dur;

    /* Constants */
    lpfloat_t log1, log01, log001;
    int samplerate;

    /* The remaining comments in this struct were copied 
     * from the original implementation. */

    /* attack and decay coefficients for the amplitude tracker. */
    lpfloat_t rise_coef, fall_coef;

    /* previous output value from the amplitude tracker */
    lpfloat_t prev_amp;

    /* coefficients for the parallel smoothers */
    lpfloat_t slow_lag_coef, fast_lag_coef;

    /* previous values of parallel smoother outputs, used in calculations */
    lpfloat_t slow_lag_prev, fast_lag_prev;

    lpfloat_t current_avg;
    lpfloat_t avg_lag_prev;
    long current_index;
    lpfloat_t avg_trig;

    int e_time;
    int gate;
} lpcoyote_t;

typedef struct lpcrossingfollower_t {
    int value;
    int lastsign;

    /* This will be 1 after processing 
     * if a transition was triggered */
    int in_transition;

    /* This will be 1 on a transition if 
     * the crossing count has been reset. */
    int ws_transition;

    /* num_crossings is the number of 
     * crossings that trigger a transition.
     * The default is 3. 
     * 3 is a good starting point for 
     * waveset segmentation as it roughly 
     * corresponds to one full cycle of a 
     * waveform: the start, crossing up or 
     * down in the middle, and the end.
     *
     * x     x     x  ... etc
     * _.-^-._     _ 
     *        \-_-/
     * ^ start     ^ end
     *       ^ middle
     **/
    int num_crossings;
    int crossing_count;
} lpcrossingfollower_t;

typedef struct lppeakfollower_t {
    lpfloat_t value;
    lpfloat_t last;
    lpfloat_t phase;
    lpfloat_t interval;
    int change;
} lppeakfollower_t;

typedef struct lpenvelopefollower_t {
    lpfloat_t value;
    lpfloat_t last;
    lpfloat_t phase;
    lpfloat_t interval;
} lpenvelopefollower_t;


typedef struct lpmir_crossingfollower_factory_t {
    lpcrossingfollower_t * (*create)();
    lpfloat_t (*process)(lpcrossingfollower_t *, lpfloat_t);
    void (*destroy)(lpcrossingfollower_t *);
} lpmir_crossingfollower_factory_t;

typedef struct lpmir_peakfollower_factory_t {
    lppeakfollower_t * (*create)(lpfloat_t);
    lpfloat_t (*process)(lppeakfollower_t *, lpfloat_t);
    void (*destroy)(lppeakfollower_t *);
} lpmir_peakfollower_factory_t;

typedef struct lpmir_envelopefollower_factory_t {
    lpenvelopefollower_t * (*create)(lpfloat_t);
    lpfloat_t (*process)(lpenvelopefollower_t *, lpfloat_t);
    void (*destroy)(lpenvelopefollower_t *);
} lpmir_envelopefollower_factory_t;

typedef struct lpmir_pitch_factory_t {
    lpyin_t * (*yin_create)(int, int);
    lpfloat_t (*yin_process)(lpyin_t *, lpfloat_t);
    void (*yin_destroy)(lpyin_t *);
} lpmir_pitch_factory_t;

typedef struct lpmir_onset_factory_t {
    lpcoyote_t * (*coyote_create)(int samplerate);
    lpfloat_t (*coyote_process)(lpcoyote_t * od, lpfloat_t sample);
    void (*coyote_destory)(lpcoyote_t * od);
} lpmir_onset_factory_t;


extern const lpmir_pitch_factory_t LPPitchTracker;
extern const lpmir_onset_factory_t LPOnsetDetector;
extern const lpmir_envelopefollower_factory_t LPEnvelopeFollower;
extern const lpmir_peakfollower_factory_t LPPeakFollower;
extern const lpmir_crossingfollower_factory_t LPCrossingFollower;

#endif
