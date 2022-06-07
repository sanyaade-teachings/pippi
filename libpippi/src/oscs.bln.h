#ifndef LP_BLNOSC_H
#define LP_BLNOSC_H

#include "pippicore.h"

typedef struct lpblnosc_t {
    lpfloat_t phase;
    lpfloat_t phaseinc;
    lpfloat_t freq;
    lpfloat_t minfreq;
    lpfloat_t maxfreq;
    lpfloat_t samplerate;
    lpbuffer_t * buf;
    int gate;
} lpblnosc_t;

typedef struct lpblnosc_factory_t {
    lpblnosc_t * (*create)(lpbuffer_t *, lpfloat_t, lpfloat_t);
    lpfloat_t (*process)(lpblnosc_t *);
    lpbuffer_t * (*render)(lpblnosc_t *, size_t, lpbuffer_t *, int);
    void (*destroy)(lpblnosc_t *);
} lpblnosc_factory_t;

extern const lpblnosc_factory_t LPBLNOsc;

#endif
