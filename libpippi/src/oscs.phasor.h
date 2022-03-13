#ifndef LP_PHASOROSC_H
#define LP_PHASOROSC_H

#include "pippicore.h"

typedef struct lpphasorosc_t {
    lpfloat_t phase;
    lpfloat_t freq;
    lpfloat_t samplerate;
} lpphasorosc_t;

typedef struct lpphasorosc_factory_t {
    lpphasorosc_t * (*create)(void);
    lpfloat_t (*process)(lpphasorosc_t *);
    lpbuffer_t * (*render)(lpphasorosc_t*, size_t, lpbuffer_t *, lpbuffer_t *, int);
    void (*destroy)(lpphasorosc_t *);
} lpphasorosc_factory_t;

extern const lpphasorosc_factory_t LPPhasorOsc;

#endif
