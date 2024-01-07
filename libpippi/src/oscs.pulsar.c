#include "oscs.pulsar.h"

#define DEBUG 0

#if DEBUG
#include <errno.h>
#endif


lpfloat_t get_stack_value(
    lpfloat_t * stack, 
    lpfloat_t phase, 
    size_t region_offset,
    size_t region_length
) {
#if DEBUG
    assert(phase >= 0);
    assert(region_length > 0);
#endif

    size_t region_index;
    lpfloat_t a, b, frac;

    phase *= region_length;
    region_index = (size_t)phase;
    frac = phase - (size_t)phase;

    a = stack[(region_index % region_length) + region_offset];
    b = stack[((region_index+1) % region_length) + region_offset];

    return (1.0f - frac) * a + (frac * b);
}

void burst_table_from_bytes(lppulsarosc_t * osc, unsigned char * bytes, size_t burst_size) {
    int mask;
    size_t i, c, pos;
    bool * burst_table;
    size_t num_bytes = burst_size / sizeof(unsigned char) + 1;

    burst_table = (bool *)LPMemoryPool.alloc(burst_size, sizeof(bool));

    pos = 0;
    for(i=0; i < num_bytes; i++) {
        for(c=0; c < sizeof(unsigned char); c++) {
            mask = 1 << c;
            burst_table[pos] = (bool)((bytes[i] & mask) >> c);
            pos += 1;
            if(pos >= burst_size) break;
        }
    }

    osc->burst = burst_table;
    osc->burst_size = burst_size;
}

void burst_table_from_file(lppulsarosc_t * osc, char * filename, size_t burst_size) {
    int fp;
    size_t num_bytes = burst_size / sizeof(unsigned char) + 1;
    unsigned char burst_buffer[num_bytes] = {};

    fp = open(filename, O_RDONLY);
    if(fp < 0) {
#if DEBUG
        printf("could not read burst file %s (%d)\n", strerror(errno), errno);
#endif
        return;
    }

    if(read(fp, burst_buffer, num_bytes) < 0) {
#if DEBUG
        printf("Could not copy burst data %s (%d)\n", strerror(errno), errno);
#endif
        return;
    }

    burst_table_from_bytes(osc, burst_buffer, burst_size);
}

lppulsarosc_t * create_pulsarosc(
    int num_wavetables, 
    lpfloat_t * wavetables, 
    size_t wavetable_length,
    size_t * wavetable_onsets,
    size_t * wavetable_lengths,

    int num_windows, 
    lpfloat_t * windows, 
    size_t window_length,
    size_t * window_onsets,
    size_t * window_lengths
) {
    lppulsarosc_t * p = (lppulsarosc_t *)LPMemoryPool.alloc(1, sizeof(lppulsarosc_t));

    p->wavetables = wavetables;
    p->wavetable_length = wavetable_length;
    p->num_wavetables = num_wavetables;
    p->wavetable_onsets = wavetable_onsets;
    p->wavetable_lengths = wavetable_lengths;

    p->windows = windows;
    p->window_length = window_length;
    p->num_windows = num_windows;
    p->window_onsets = window_onsets;
    p->window_lengths = window_lengths;

    p->burst = NULL;
    p->burst_pos = 0;
    p->burst_size = 0;

    p->pulse_edge = 0;
    p->saturation = 1.f;
    p->pulsewidth = 1.f;
    p->samplerate = DEFAULT_SAMPLERATE;
    p->freq = 220.0;
    p->wavetable_morph_freq = .12f;
    p->window_morph_freq = .12f;

    return p;
}

lpfloat_t process_pulsarosc(lppulsarosc_t * p) {
    lpfloat_t ipw, isr, sample, mod, a, b, 
              wtmorphpos, wtmorphfrac,
              winmorphpos, winmorphfrac;
    int wavetable_index, window_index;
    int burst;

#if DEBUG
    assert(p->wavetables != NULL);
    assert(p->num_wavetables > 0);
    assert(p->windows != NULL);
    assert(p->num_windows > 0);
#endif

    assert(p->samplerate > 0);
    wavetable_index = 0;
    window_index = 0;
    isr = 1.f / (lpfloat_t)p->samplerate;
    ipw = 1.f;
    sample = 0.f;
    mod = 0.f;
    burst = 1;

    /* Store the inverse pulsewidth if non-zero */
    if(p->pulsewidth > 0) ipw = 1.0/p->pulsewidth;

    /* Look up the burst value -- NULL burst is always on. 
     * In other words, bursting only happens when the burst 
     * table is non-NULL. Otherwise all pulses sound. */
    if(p->burst != NULL && p->burst_size > 0) burst = p->burst[p->burst_pos % p->burst_size];

    /* Override burst if desaturation is triggered */
    if(p->saturation < 1.f && LPRand.rand(0.f, 1.f) > p->saturation) {
        burst = 0; 
    }

    //printf("pw=%f/ipw=%f\tburst=%d\tburst_pos=%d\n", (float)p->pulsewidth, (float)ipw, (int)burst, (int)p->burst_pos);

    /* If there's a non-zero pulsewidth, and the burst value is 1, 
     * then syntesize a pulse */
    if(ipw > 0 && burst) {
        if(p->num_wavetables == 1) {
            sample = get_stack_value(p->wavetables, p->phase, 0, p->wavetable_length);
        } else {
            wtmorphpos = p->wavetable_morph * p->num_wavetables;
            wavetable_index = (int)wtmorphpos;
            wtmorphfrac = wtmorphpos - wavetable_index;

            a = get_stack_value(p->wavetables, p->phase,
                p->wavetable_onsets[wavetable_index],
                p->wavetable_lengths[wavetable_index]
            );

            b = get_stack_value(p->wavetables, p->phase,
                p->wavetable_onsets[(wavetable_index+1) % p->num_wavetables],
                p->wavetable_lengths[(wavetable_index+1) % p->num_wavetables]
            );

            sample = (1.f - wtmorphfrac) * a + (wtmorphfrac * b);
        }

        if(p->num_windows == 1) {
            mod = get_stack_value(p->windows, p->phase, 0, p->window_length);
        } else {
            winmorphpos = p->window_morph * p->num_windows;
            window_index = (int)winmorphpos;
            winmorphfrac = winmorphpos - window_index;

            a = get_stack_value(p->windows, p->phase,
                p->window_onsets[window_index], 
                p->window_lengths[window_index]
            );

            b = get_stack_value(p->windows, p->phase,
                p->window_onsets[(window_index+1) % p->num_windows],
                p->window_lengths[(window_index+1) % p->num_windows]
            );

            mod = (1.f - winmorphfrac) * a + (winmorphfrac * b);
        }
    } 

    // increment phases
    p->wavetable_morph += isr * p->wavetable_morph_freq;
    p->window_morph += isr * p->window_morph_freq;
    p->phase += isr * p->freq;
    if(p->burst != NULL && p->phase >= 1.f) p->burst_pos += 1;

    // Set the pulse boundry flag so external programs can know
    // about phase boundries (and do things when they happen)
    p->pulse_edge = (p->phase >= 1.f);

    // wrap phases
    while(p->phase >= 1.f) p->phase -= 1.f;
    while(p->wavetable_morph >= 1.f) p->wavetable_morph -= 1.f;
    while(p->window_morph >= 1.f) p->window_morph -= 1.f;
    if(p->burst_size > 0) while(p->burst_pos >= p->burst_size) p->burst_pos -= p->burst_size;

    return sample * mod;
}

void destroy_pulsarosc(lppulsarosc_t* p) {
    LPMemoryPool.free(p);
}


const lppulsarosc_factory_t LPPulsarOsc = { create_pulsarosc, burst_table_from_file, burst_table_from_bytes, process_pulsarosc, destroy_pulsarosc };
