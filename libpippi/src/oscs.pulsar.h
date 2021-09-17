#ifndef LP_PULSAR_H
#define LP_PULSAR_H

#include "pippicore.h"

typedef struct lppulsarosc_t {
    lpstack_t * wts;   /* Wavetable stack */
    lpstack_t * wins;  /* Window stack */
    lparray_t * burst;    /* Burst table - null always on */

    lpfloat_t saturation; /* Probability of all pulses to no pulses */
    lpfloat_t pulsewidth;
    lpfloat_t samplerate;
    lpfloat_t freq;
} lppulsarosc_t;

typedef struct lppulsarosc_factory_t {
    lppulsarosc_t * (*create)(void);
    lpfloat_t (*process)(lppulsarosc_t *);
    void (*destroy)(lppulsarosc_t*);
} lppulsarosc_factory_t;

extern const lppulsarosc_factory_t LPPulsarOsc;

#endif
