#include "oscs.pulsar.h"

#define DEBUG 1

// get stack value at phase


lpfloat_t get_stack_value(
    lpfloat_t * stack, 
    lpfloat_t phase, 
    size_t stack_length, 
    size_t region_offset,
    size_t region_length
) {
    assert(phase >= 0);
    assert(stack_length > 0);
    assert(region_length > 0);

    size_t region_index;
    lpfloat_t a, b, frac;

    //if(DEBUG) printf("IN stack_length=%d, region_offset=%d, region_length=%d, phase=%f\n", (int)stack_length, (int)region_offset, (int)region_length, (float)phase);

    phase *= region_length;
    region_index = (size_t)phase;
    frac = phase - (size_t)phase;

    //if(DEBUG) printf("TO stack_length=%d, region_offset=%d, region_length=%d, phase=%f, frac=%f\n", (int)stack_length, (int)region_offset, (int)region_length, (float)phase, (float)frac);

    a = stack[(region_index % region_length) + region_offset];
    b = stack[((region_index+1) % region_length) + region_offset];
    //printf("   a=%f b=%f\n", (float)a, (float)b);
    //printf("\n");

    return (1.0f - frac) * a + (frac * b);
}

int read_burst_value(void * burst, size_t burst_pos) {
    size_t burst_buffer_size = burst_pos / sizeof(unsigned int) + 1;
    unsigned int * burst_source;
    unsigned int burst_buffer[burst_buffer_size];
    int burst_mask;

    if(burst == NULL) return 1;

    /* Copy the memory area to a local buffer */
    burst_source = (unsigned int *)burst;
    memcpy(burst_buffer, burst_source, burst_buffer_size);

    /* Find the bit position at the burst_pos */
    burst_mask = 1 << burst_pos;
    return (*burst_source & burst_mask) >> burst_pos;
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
    int i;
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

    p->saturation = 1.f;
    p->pulsewidth = 1.f;
    p->samplerate = DEFAULT_SAMPLERATE;
    p->freq = 220.0;
    p->wavetable_morph_freq = .12f;
    p->window_morph_freq = .12f;

    printf("num wavetables %d\n", (int)num_wavetables);
    for(i=0; i < num_wavetables; i++) {
        printf("onset %d\n", (int)wavetable_onsets[i]);
        printf("length %d\n", (int)wavetable_lengths[i]);
    }

    printf("num windows %d\n", (int)num_windows);
    for(i=0; i < num_windows; i++) {
        printf("onset %d\n", (int)window_onsets[i]);
        printf("length %d\n", (int)window_lengths[i]);
    }

    return p;
}

lpfloat_t process_pulsarosc(lppulsarosc_t * p) {
    lpfloat_t ipw, isr, sample, mod, a, b, 
              wtmorphpos, wtmorphfrac,
              winmorphpos, winmorphfrac;
    int wavetable_index, window_index;
    //int burst;

    assert(p->wavetables != NULL);
    assert(p->num_wavetables > 0);
    assert(p->windows != NULL);
    assert(p->num_windows > 0);

    wavetable_index = 0;
    window_index = 0;
    isr = 1.f / (lpfloat_t)p->samplerate;
    ipw = 1.f;
    sample = 0.f;
    mod = 0.f;
    //burst = 1;

    /* Store the inverse pulsewidth if non-zero */
    if(p->pulsewidth > 0) ipw = 1.0/p->pulsewidth;

    /* Look up the burst value -- NULL burst is always on */
    //burst = read_burst_value(p->burst, p->burst_pos);

    /* Override burst if desaturation is triggered */
    //if(p->saturation < 1.f && LPRand.rand(0.f, 1.f) > p->saturation) {
    //    burst = 0; 
    //}

    /* If there's a non-zero pulsewidth, and the burst value is 1, 
     * then syntesize a pulse */
    if(ipw > 0) {
        //printf("wavetable\n");
        if(p->num_wavetables == 1) {
            sample = get_stack_value(p->wavetables, p->phase, p->wavetable_length, 0, p->wavetable_length);
        } else {
            wtmorphpos = p->wavetable_morph * p->num_wavetables;
            wavetable_index = (int)wtmorphpos;
            wtmorphfrac = wtmorphpos - wavetable_index;
            //if(DEBUG) printf("wavetable index: %i (num wavetables %i)\n", wavetable_index, p->num_wavetables);

            a = get_stack_value(p->wavetables, p->phase, p->wavetable_length,
                p->wavetable_onsets[wavetable_index],
                p->wavetable_lengths[wavetable_index]
            );

            b = get_stack_value(p->wavetables, p->phase, p->wavetable_length,
                p->wavetable_onsets[(wavetable_index+1) % p->num_wavetables],
                p->wavetable_lengths[(wavetable_index+1) % p->num_wavetables]
            );

            sample = (1.f - wtmorphfrac) * a + (wtmorphfrac * b);
        }

        //printf("window\n");
        if(p->num_windows == 1) {
            mod = get_stack_value(p->windows, p->phase, p->window_length, 0, p->window_length);
            //if(DEBUG) printf("window index: %i (num windows %i, window length %d)\n", window_index, p->num_windows, p->window_length);
        } else {
            winmorphpos = p->window_morph * p->num_windows;
            window_index = (int)winmorphpos;
            winmorphfrac = winmorphpos - window_index;
            //if(DEBUG) printf("window index: %i (num windows %i)\n", window_index, p->num_windows);

            a = get_stack_value(p->windows, p->phase, p->window_length, 
                p->window_onsets[window_index], 
                p->window_lengths[window_index]
            );

            b = get_stack_value(p->windows, p->phase, p->window_length,
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
    //if(p->burst != NULL && p->phase >= 1.f) p->burst_pos += 1;
    
    if(p->phase >= 1.f) {
        //printf("phase overflow %f\n", (float)p->phase);
        //if(DEBUG) printf("wavetables=%d windows=%d\n", (int)p->num_wavetables, (int)p->num_windows);
        //if(DEBUG) printf("sample = %f\tmod = %f\n", (float)sample, (float)mod);
        //if(DEBUG) printf("\n");
        //printf("sample %f mod %f\n", sample, mod);
    }

    // wrap phases
    while(p->phase >= 1.f) p->phase -= 1.f;
    while(p->wavetable_morph >= 1.f) p->wavetable_morph -= 1.f;
    while(p->window_morph >= 1.f) p->window_morph -= 1.f;
    //while(p->burst_pos >= p->burst_size) p->burst_pos -= p->burst_size;

    return sample * mod;
}

void destroy_pulsarosc(lppulsarosc_t* p) {
    LPMemoryPool.free(p);
}


const lppulsarosc_factory_t LPPulsarOsc = { create_pulsarosc, process_pulsarosc, destroy_pulsarosc };
