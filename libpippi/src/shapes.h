#ifndef LP_SHAPES_H
#define LP_SHAPES_H

#include "pippicore.h"

typedef struct lpshapes_t {
    lpbuffer_t * wt;
    lpfloat_t density;
    lpfloat_t periodicity;
    lpfloat_t stability;
    lpfloat_t phase;
    lpfloat_t freq;
    lpfloat_t minfreq;
    lpfloat_t maxfreq;
    lpfloat_t samplerate;
} lpshapes_t;

typedef struct lpshapes_factory_t {
    lpshapes_t * (*win)(int base);
    lpshapes_t * (*wt)(int base);
    lpfloat_t (*process)(lpshapes_t * s);
    void (*destroy)(lpshapes_t * s);
} lpshapes_factory_t;

extern const lpshapes_factory_t LPShapes;

#endif
