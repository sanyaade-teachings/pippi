#include "oscs.pulsar.h"

#ifndef DEBUG
#define DEBUG 1
#endif
#if DEBUG
#include <errno.h>
#endif


lpfloat_t get_stack_value(
    lpbuffer_t * stack, 
    lpfloat_t phase, 
    size_t region_offset,
    size_t region_length
) {
#if DEBUG
    assert(phase >= 0);
    assert(region_length > 0);
    assert(stack != NULL);
    assert(stack->data != NULL);
#endif

    size_t region_index, ai, bi;
    lpfloat_t a, b, frac;

    phase *= region_length;
    region_index = (size_t)phase;
    frac = phase - (size_t)phase;

    region_index = region_index % region_length;

    ai = (region_index+region_offset) % stack->length;
    bi = (region_index+region_offset+1) % stack->length;

    a = stack->data[ai];
    b = stack->data[bi];

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
    unsigned char burst_buffer[num_bytes];

    memset(burst_buffer, 0, num_bytes * sizeof(unsigned char));

    fp = open(filename, O_RDONLY);
    if(fp < 0) {
#if DEBUG
        printf("could not read burst file %s (%d)\n", strerror(errno), errno);
#endif
        close(fp);
        return;
    }

    if(read(fp, burst_buffer, num_bytes) < 0) {
#if DEBUG
        printf("Could not copy burst data %s (%d)\n", strerror(errno), errno);
#endif
        close(fp);
        return;
    }

    close(fp);

    burst_table_from_bytes(osc, burst_buffer, burst_size);
}


void create_pulsarosc_wavetable_stack(lppulsarosc_t * p, int numtables, va_list vl) {
    LPMemoryPool.free(p->wavetables);
    LPMemoryPool.free(p->wavetable_onsets);
    LPMemoryPool.free(p->wavetable_lengths);

    if(numtables == 0) return;

    p->num_wavetables = numtables;
    p->wavetable_onsets = (size_t *)LPMemoryPool.alloc(numtables, sizeof(size_t));
    p->wavetable_lengths = (size_t *)LPMemoryPool.alloc(numtables, sizeof(size_t));
    p->wavetables = lpbuffer_create_stack(LPWavetable.create, numtables, p->wavetable_onsets, p->wavetable_lengths, vl);
}

void create_pulsarosc_window_stack(lppulsarosc_t * p, int numtables, va_list vl) {
    LPMemoryPool.free(p->windows);
    LPMemoryPool.free(p->window_onsets);
    LPMemoryPool.free(p->window_lengths);

    if(numtables == 0) return;

    p->num_windows = numtables;
    p->window_onsets = (size_t *)LPMemoryPool.alloc(numtables, sizeof(size_t));
    p->window_lengths = (size_t *)LPMemoryPool.alloc(numtables, sizeof(size_t));
    p->windows = lpbuffer_create_stack(LPWindow.create, numtables, p->window_onsets, p->window_lengths, vl);
}


lppulsarosc_t * create_pulsarosc(int num_wavetables, int num_windows, ...) {
    va_list vl, vp;
    lppulsarosc_t * p = (lppulsarosc_t *)LPMemoryPool.alloc(1, sizeof(lppulsarosc_t));

    va_start(vl, num_windows);
    create_pulsarosc_wavetable_stack(p, num_wavetables, vl);
    va_copy(vp, vl);
    create_pulsarosc_window_stack(p, num_windows, vp);
    va_end(vp);
    va_end(vl);

    p->wavetable_morph_freq = num_wavetables > 0 ? .12f : 0;
    p->window_morph_freq = num_windows > 0 ? .12f : 0;

    p->burst = NULL;
    p->burst_pos = 0;
    p->burst_size = 0;

    p->pulse_edge = 0;
    p->phase = 0.f;
    p->saturation = 1.f;
    p->pulsewidth = 1.f;
    p->samplerate = DEFAULT_SAMPLERATE;
    p->freq = 220.0;

    return p;
}

lpfloat_t process_pulsarosc(lppulsarosc_t * p) {
    lpfloat_t ipw, isr, sample, a, b, 
              wtmorphpos, wtmorphfrac,
              winmorphpos, winmorphfrac;
    int wavetable_index, window_index;
    int burst;

    assert(p->samplerate > 0);
    wavetable_index = 0;
    window_index = 0;
    isr = 1.f / (lpfloat_t)p->samplerate;
    ipw = 1.f;
    sample = 0.f;
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

    /* If there's a non-zero pulsewidth, and the burst value is 1, 
     * then syntesize a pulse */
    if(ipw > 0 && burst) {
        sample = 1.f; // When num_wavetables == 0, the window functions can be used like an LFO
        if(p->num_wavetables == 1) {
            sample = get_stack_value(p->wavetables, p->phase, 0, p->wavetables->length);
        } else if(p->num_wavetables > 0) {
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

        // When num_windows == 0, this is a noop
        if(p->num_windows == 1) {
            sample *= get_stack_value(p->windows, p->phase, 0, p->windows->length);
        } else if(p->num_windows > 0) {
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

            sample *= (1.f - winmorphfrac) * a + (winmorphfrac * b);
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
    if(p->phase < 0) {
        while(p->phase < 0) p->phase += 1.f;
    } else if(p->phase >= 1.f) {
        while(p->phase >= 1.f) p->phase -= 1.f;
    }

    while(p->wavetable_morph >= 1.f) p->wavetable_morph -= 1.f;
    while(p->window_morph >= 1.f) p->window_morph -= 1.f;
    if(p->burst_size > 0) while(p->burst_pos >= p->burst_size) p->burst_pos -= p->burst_size;

    return sample;
}

void destroy_pulsarosc(lppulsarosc_t* p) {
    LPMemoryPool.free(p);
}


const lppulsarosc_factory_t LPPulsarOsc = { create_pulsarosc, burst_table_from_file, burst_table_from_bytes, process_pulsarosc, destroy_pulsarosc };
