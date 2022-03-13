#ifndef LP_TABLEOSC_H
#define LP_TABLEOSC_H

#include "pippicore.h"

typedef struct lptableosc_t {
    lpfloat_t phase;
    lpfloat_t phaseinc;
    lpfloat_t freq;
    lpfloat_t samplerate;
    lpbuffer_t * buf;
    int gate;
} lptableosc_t;

typedef struct lptableosc_factory_t {
    lptableosc_t * (*create)(lpbuffer_t *);
    lpfloat_t (*process)(lptableosc_t *);
    lpbuffer_t * (*render)(lptableosc_t *, size_t, lpbuffer_t *, int);
    void (*destroy)(lptableosc_t *);
} lptableosc_factory_t;

extern const lptableosc_factory_t LPTableOsc;

#endif
