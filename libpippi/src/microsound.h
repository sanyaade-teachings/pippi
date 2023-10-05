#ifndef LP_GRAINS_H
#define LP_GRAINS_H

#include "pippicore.h"
#include "oscs.tape.h"

#define LPFORMATION_MAXGRAINS 512

typedef struct lpgrain_t {
    size_t length;
    int channels;
    lpfloat_t pulsewidth; 

    size_t range;
    size_t start;
    size_t offset;

    lpfloat_t phase_offset;
    lpfloat_t phase;
    lpfloat_t pan;
    lpfloat_t amp;
    lpfloat_t speed;
    lpfloat_t skew; /* phase distortion on the grain window */

    int in_use;
    int gate;

    lpbuffer_t * buf;
    lpbuffer_t * window;
} lpgrain_t;

typedef struct lpformation_t {
    lpgrain_t grains[LPFORMATION_MAXGRAINS];
    int num_active_grains;
    size_t numlayers;
    size_t grainlength;
    lpfloat_t grainlength_maxjitter;
    lpfloat_t grainlength_jitter; /* 0-1 proportional to grainlength_maxjitter */
    size_t graininterval;

    lpfloat_t spread; /* pan spread */
    lpfloat_t speed;
    lpfloat_t scrub;
    lpfloat_t offset;
    lpfloat_t skew;
    lpfloat_t amp;
    lpfloat_t pan;
    lpfloat_t pulsewidth; 

    lpfloat_t phase; /* internal phase */
    lpfloat_t phase_inc;
    lpfloat_t pos; /* sample start position in source buffer: 0-1 */

    lpfloat_t onset_phase;
    lpfloat_t onset_phase_inc;

    lpbuffer_t * window;
    lpbuffer_t * current_frame;
    lpbuffer_t * rb;
} lpformation_t;

typedef struct lpformation_factory_t {
    lpformation_t * (*create)(int window_type, int numlayers, size_t grainlength, size_t rblength, int channels, int samplerate, lpbuffer_t * user_window);
    void (*process)(lpformation_t *);
    void (*destroy)(lpformation_t *);
} lpformation_factory_t;

extern const lpformation_factory_t LPFormation;

#endif
