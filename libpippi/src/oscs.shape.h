#ifndef LP_SHAPEOSC_H
#define LP_SHAPEOSC_H

#include "pippicore.h"

typedef struct lpshapeosc_t {
    lpbuffer_t * wt;
    lpfloat_t density;
    lpfloat_t periodicity;
    lpfloat_t stability;
    lpfloat_t phase;
    lpfloat_t freq;
    lpfloat_t minfreq;
    lpfloat_t maxfreq;
    lpfloat_t samplerate;

    lpfloat_t wtmin;
    lpfloat_t wtmax;
    lpfloat_t min;
    lpfloat_t max;
} lpshapeosc_t;

typedef struct lpmultishapeosc_t {
    int numshapeosc;
    lpshapeosc_t ** shapeosc;
    lpfloat_t density;
    lpfloat_t periodicity;
    lpfloat_t stability;
    lpfloat_t minfreq;
    lpfloat_t maxfreq;
    lpfloat_t samplerate;
    lpfloat_t min;
    lpfloat_t max;
} lpmultishapeosc_t;

typedef struct lpshapeosc_factory_t {
    lpshapeosc_t * (*create)(lpbuffer_t * wt);
    lpmultishapeosc_t * (*multi)(int numshapeosc, ...);
    lpfloat_t (*process)(lpshapeosc_t * s);
    lpfloat_t (*multiprocess)(lpmultishapeosc_t * m);
    void (*destroy)(lpshapeosc_t * s);
    void (*multidestroy)(lpmultishapeosc_t * m);
} lpshapeosc_factory_t;

extern const lpshapeosc_factory_t LPShapeOsc;

#endif
