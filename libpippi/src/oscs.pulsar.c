#include "oscs.pulsar.h"

lpfloat_t interpolate_waveset(lpfloat_t * buf, lpfloat_t phase, size_t length) {
    lpfloat_t frac, a, b;
    size_t i, boundry;

    boundry = length - 1;

    if(length < 2) return buf[0];
    
    frac = phase - (int)phase;
    i = (int)phase;

    if (i >= boundry) return 0;

    a = buf[i];
    b = buf[i+1];

    return (1.0f - frac) * a + (frac * b);
}


lppulsarosc_t * create_pulsarosc(
    int num_wavetables, 
    lpfloat_t * wavetables, 
    size_t * wavetable_onsets,
    size_t * wavetable_lengths,

    int num_windows, 
    lpfloat_t * windows, 
    size_t * window_onsets,
    size_t * window_lengths
) {
    lppulsarosc_t * p = (lppulsarosc_t *)LPMemoryPool.alloc(1, sizeof(lppulsarosc_t));

    p->wavetables = wavetables;
    p->num_wavetables = num_wavetables;
    p->wavetable_onsets = wavetable_onsets;
    p->wavetable_lengths = wavetable_lengths;

    p->windows = windows;
    p->num_windows = num_windows;
    p->window_onsets = window_onsets;
    p->window_lengths = window_lengths;

    p->burst = NULL;

    p->saturation = 1.f;
    p->pulsewidth = 1.f;
    p->samplerate = DEFAULT_SAMPLERATE;
    p->freq = 220.0;

    return p;
}

lpfloat_t process_pulsarosc(lppulsarosc_t * p) {
    lpfloat_t ipw, isr, sample, mod, burst, a, b, 
              wavetable_phase, window_phase,
              wtmorphpos, wtmorphfrac,
              winmorphpos, winmorphfrac;
    int wtmorphidx, wtmorphmul,
        winmorphidx, winmorphmul,
        wavetable_index, window_index;
    size_t wavetable_length, window_length;

    assert(p->wavetables != NULL);
    assert(p->num_wavetables > 0);
    assert(p->windows != NULL);
    assert(p->num_windows > 0);

    wavetable_index = 0;
    wavetable_phase = 0.f;
    window_index = 0;
    window_phase = 0.f;
    ipw = 0.f;
    sample = 0.f;
    mod = 0.f;
    burst = 1.f;
    isr = 1.f / p->samplerate;

    /* Store the inverse pulsewidth if non-zero */
    if(p->pulsewidth > 0) ipw = 1.0/p->pulsewidth;

    /* Look up the burst value if there's a burst table */
    if(p->burst != NULL) {
        
    }

    /*
    if(p->burst != NULL && p->burst->data != NULL && p->burst->phase < p->burst->length) {
        burst = p->burst->data[(int)p->burst->phase];
    }
    */

    /* Override burst if desaturation is triggered */
    if(p->saturation < 1.f && LPRand.rand(0.f, 1.f) > p->saturation) {
        burst = 0; 
    }

    /* If there's a non-zero pulsewidth, and the burst value is 1, 
     * then syntesize a pulse */
    if(ipw > 0 && burst > 0) {
        if(p->num_wavetables == 1) {
            sample = interpolate_waveset(p->wavetables, wavetable_phase * ipw, p->wavetable_lengths[0]);
        } else {
            wavetable_length = p->wavetable_lengths[wavetable_index];
            wtmorphmul = wavetable_length-1 > 1 ? wavetable_length-1 : 1;
            wtmorphpos = lpwv(p->wavetable_positions[wavetable_index], 0, 1) * wtmorphmul;
            wavetable_index = (int)wtmorphpos;
            wtmorphfrac = wtmorphpos - wavetable_index;

            a = LPInterpolation.linear_pos2(&p->wavetables[wavetable_index], p->wavetable_lengths[wavetable_index], p->wavetable_phase * ipw);
            b = LPInterpolation.linear_pos2(&p->wavetables[wtmorphidx+1], p->wavetable_lengths[wtmorphidx+1], p->wavetable_phase * ipw);
            sample = (1.0 - wtmorphfrac) * a + (wtmorphfrac * b);
        }

        if(p->num_windows == 1) {
            mod = interpolate_waveset(p->windows, window_phase * ipw, p->window_lengths[0]);
        } else {
            window_length = p->window_lengths[window_index];
            winmorphmul = window_length-1 > 1 ? window_length-1 : 1;
            winmorphpos = lpwv(p->window_positions[window_index], 0, 1) * winmorphmul;
            window_index = (int)winmorphpos;
            winmorphfrac = winmorphpos - window_index;

            a = LPInterpolation.linear_pos2(&p->windows[window_index], p->window_lengths[window_index], p->window_phase * ipw);
            b = LPInterpolation.linear_pos2(&p->windows[winmorphidx+1], p->window_lengths[winmorphidx+1], p->window_phase * ipw);
            mod = (1.0 - winmorphfrac) * a + (winmorphfrac * b);
        }
    } 

    p->wavetable_phase += isr * p->freq;
    p->window_phase += isr * p->freq;

    /*
    if(p->burst != NULL && window_phase >= p->window_lengths[window_index]) {
        p->burst->phase += 1;
    }
    */

    if(wavetable_phase >= 1.f) wavetable_phase -= 1.f;
    if(window_phase >= 1.f) window_phase -= 1.f;
    //if(p->burst != NULL && p->burst->phase >= p->burst->length) p->burst->phase -= p->burst->length;

    return sample * mod;
}

void destroy_pulsarosc(lppulsarosc_t* p) {
    LPMemoryPool.free(p->wavetables);
    LPMemoryPool.free(p->windows);
    LPArray.destroy(p->burst);
    LPMemoryPool.free(p);
}


const lppulsarosc_factory_t LPPulsarOsc = { create_pulsarosc, process_pulsarosc, destroy_pulsarosc };
