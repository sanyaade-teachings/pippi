#ifndef LP_GRAINS_H
#define LP_GRAINS_H

#include "pippicore.h"
#include "oscs.tape.h"

#define LPFORMATION_MAXGRAINS 512

typedef struct lpgrain_t {
    size_t length;
    int channels;
    lpfloat_t samplerate; 
    lpfloat_t pulsewidth; 
    lpfloat_t grainlength;
    lpfloat_t offset; /* in seconds */

    lpfloat_t pan;
    lpfloat_t amp;
    lpfloat_t speed;
    lpfloat_t skew; /* phase distortion on the grain window */

    int gate;

    lptapeosc_t * src;
    lptapeosc_t * win;
} lpgrain_t;

typedef struct lpformation_t {
    lpgrain_t grains[LPFORMATION_MAXGRAINS];
    int numgrains;
    lpfloat_t grainlength;
    lpfloat_t grainlength_maxjitter;
    lpfloat_t grainlength_jitter; /* 0-1 proportional to grainlength_maxjitter */
    lpfloat_t grid_maxjitter;
    lpfloat_t grid_jitter;

    lpfloat_t spread; /* pan spread */
    lpfloat_t speed;
    lpfloat_t offset; /* in seconds */
    lpfloat_t skew;
    lpfloat_t amp;
    lpfloat_t pan;
    lpfloat_t pulsewidth; 

    lpbuffer_t * source;
    lpbuffer_t * window;
    lpbuffer_t * current_frame;
} lpformation_t;

typedef struct lpformation_factory_t {
    lpformation_t * (*create)(int numgrains, lpbuffer_t * src, lpbuffer_t * win);
    void (*process)(lpformation_t *);
    void (*destroy)(lpformation_t *);
} lpformation_factory_t;

void grain_init(lpgrain_t * grain, lpbuffer_t * src, lpbuffer_t * win);
void grain_process(lpgrain_t * g, lpbuffer_t * out);

extern const lpformation_factory_t LPFormation;

#endif
