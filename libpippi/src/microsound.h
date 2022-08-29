#ifndef LP_GRAINS_H
#define LP_GRAINS_H

#include "pippicore.h"
#include "oscs.tape.h"

typedef struct lpgrain_t {
    size_t length;
    lpfloat_t pulsewidth; 

    size_t range;
    size_t start;
    size_t offset;

    lpfloat_t phase;
    lpfloat_t pan;
    lpfloat_t amp;
    lpfloat_t speed;
    lpfloat_t skew; /* phase distortion on the grain window */

    int gate;

    lpbuffer_t * buf;
    lpbuffer_t * window;
    lpbuffer_t * current_frame;
} lpgrain_t;

typedef struct lpformation_t {
    lpgrain_t ** grains;
    size_t numgrains;
    size_t maxlength;
    size_t minlength;
    lpfloat_t spread; /* pan spread */
    lpfloat_t speed;
    lpfloat_t scrub;
    lpfloat_t offset;
    lpfloat_t skew;
    lpfloat_t grainamp;
    lpfloat_t pulsewidth; 
    lpbuffer_t * window;
    lpbuffer_t * current_frame;
    lpbuffer_t * rb;
} lpformation_t;

typedef struct lpgrain_factory_t {
    lpgrain_t * (*create)(lpfloat_t, lpbuffer_t *, lpbuffer_t *);
    void (*process)(lpgrain_t *, lpbuffer_t *);
    void (*destroy)(lpgrain_t *);
} lpgrain_factory_t;

typedef struct lpformation_factory_t {
    lpformation_t * (*create)(int, size_t, size_t, size_t, int, int);
    void (*process)(lpformation_t *);
    void (*destroy)(lpformation_t *);
} lpformation_factory_t;

extern const lpgrain_factory_t LPGrain;
extern const lpformation_factory_t LPFormation;

#endif
