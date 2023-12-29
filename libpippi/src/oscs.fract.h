#ifndef LP_FRACTOSC_H
#define LP_FRACTOSC_H

#include "pippicore.h"

typedef struct lpfractosc_t {
    lpfloat_t phase;
    lpfloat_t freq;
    lpfloat_t samplerate;
    lpfloat_t depth;
} lpfractosc_t;

typedef struct lpfractosc_factory_t {
    lpfractosc_t * (*create)(void);
    lpfloat_t (*process)(lpfractosc_t *);
    lpbuffer_t * (*render)(lpfractosc_t*, size_t, lpbuffer_t *, lpbuffer_t *, int);
    void (*destroy)(lpfractosc_t *);
} lpfractosc_factory_t;

extern const lpfractosc_factory_t LPFractOsc;

#endif
