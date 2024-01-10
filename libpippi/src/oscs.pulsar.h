#ifndef LP_PULSAR_H
#define LP_PULSAR_H

#include "pippicore.h"
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct lppulsarosc_t {
    lpbuffer_t * wavetables;   /* Wavetable stack */
    int num_wavetables;
    size_t * wavetable_onsets; /* The start position for each table */
    size_t * wavetable_lengths; /* The length of each table */
    lpfloat_t wavetable_morph;
    lpfloat_t wavetable_morph_freq;

    lpbuffer_t * windows;  /* Window stack */
    int num_windows;
    size_t * window_onsets;
    size_t * window_lengths;
    lpfloat_t window_morph;
    lpfloat_t window_morph_freq;

    bool * burst;         /* Burst table - null table == pulses always on */
    size_t burst_size;
    size_t burst_pos; 

    bool pulse_edge;
    lpfloat_t phase;
    lpfloat_t saturation; /* Probability of all pulses to no pulses */
    lpfloat_t pulsewidth;
    lpfloat_t samplerate;
    lpfloat_t freq;
} lppulsarosc_t;

typedef struct lppulsarosc_factory_t {
    lppulsarosc_t * (*create)(int num_wavetables, 
        lpbuffer_t * wavetables, 
        size_t * wavetable_onsets,
        size_t * wavetable_lengths,

        int num_windows, 
        lpbuffer_t * windows, 
        size_t * window_onsets,
        size_t * window_lengths
    );
    void (*burst_file)(lppulsarosc_t * osc, char * filename, size_t burst_size);
    void (*burst_bytes)(lppulsarosc_t * osc, unsigned char * bytes, size_t burst_size);
    lpfloat_t (*process)(lppulsarosc_t *);
    void (*destroy)(lppulsarosc_t*);
} lppulsarosc_factory_t;

extern const lppulsarosc_factory_t LPPulsarOsc;

#endif
