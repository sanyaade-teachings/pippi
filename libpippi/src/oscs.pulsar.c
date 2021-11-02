#include "oscs.pulsar.h"

lppulsarosc_t * create_pulsarosc(void) {
    lppulsarosc_t * p = (lppulsarosc_t *)LPMemoryPool.alloc(1, sizeof(lppulsarosc_t));

    p->wts = NULL;
    p->wins = NULL;
    p->burst = NULL;

    p->saturation = 1.f;
    p->pulsewidth = 1.f;
    p->samplerate = DEFAULT_SAMPLERATE;
    p->freq = 220.0;

    return p;
}

lpfloat_t process_pulsarosc(lppulsarosc_t * p) {
    /* Get the pulsewidth and inverse pulsewidth if the pulsewidth 
     * is zero, skip everything except phase incrementing and return 
     * a zero down the line.
     */
    lpfloat_t ipw, isr, sample, mod, burst, a, b;
    lpfloat_t wtmorphpos, wtmorphfrac;
    lpfloat_t winmorphpos, winmorphfrac;
    int wtmorphidx, wtmorphmul;
    int winmorphidx, winmorphmul;

    /* The wavetable and window stacks must be populated by the user after creation */
    assert(p->wts != NULL);
    assert(p->wins != NULL);

    ipw = 0.f;
    sample = 0.f;
    mod = 0.f;
    burst = 1.f;
    isr = 1.f / p->samplerate;

    if(p->pulsewidth > 0) ipw = 1.0/p->pulsewidth;

    if(p->burst != NULL && p->burst->data != NULL && p->burst->phase < p->burst->length) {
        burst = p->burst->data[(int)p->burst->phase];
    }

    if(p->saturation < 1.f && LPRand.rand(0.f, 1.f) > p->saturation) {
        burst = 0; 
    }

    if(ipw > 0 && burst > 0) {
        if(p->wts->length == 1) {
            /* If there is just a single wavetable in the stack, get the current value */
            sample = LPInterpolation.linear(p->wts->stack[0], p->wts->phase * ipw);
        } else {
            /* If there are multiple wavetables in the stack, get their values  
             * and then interpolate the value at the morph position between them.
             */
            wtmorphmul = p->wts->length-1 > 1 ? p->wts->length-1 : 1;
            wtmorphpos = lpwv(p->wts->pos, 0, 1) * wtmorphmul;
            wtmorphidx = (int)wtmorphpos;
            wtmorphfrac = wtmorphpos - wtmorphidx;
            a = LPInterpolation.linear_pos(p->wts->stack[wtmorphidx], p->wts->phase * ipw);
            b = LPInterpolation.linear_pos(p->wts->stack[wtmorphidx+1], p->wts->phase * ipw);
            sample = (1.0 - wtmorphfrac) * a + (wtmorphfrac * b);
        }

        if(p->wins->length == 1) {
            /* If there is just a single window in the stack, get the current value */
            mod = LPInterpolation.linear(p->wins->stack[0], p->wins->phase * ipw);
        } else {
            /* If there are multiple wavetables in the stack, get their values 
             * and then interpolate the value at the morph position between them.
             */
            winmorphmul = p->wins->length-1 > 1 ? p->wins->length-1 : 1;
            winmorphpos = p->wins->pos * winmorphmul;
            winmorphidx = (int)winmorphpos;
            winmorphfrac = winmorphpos - winmorphidx;
            a = LPInterpolation.linear_pos(p->wins->stack[winmorphidx], p->wins->phase * ipw);
            b = LPInterpolation.linear_pos(p->wins->stack[winmorphidx+1], p->wins->phase * ipw);
            mod = (1.0 - winmorphfrac) * a + (winmorphfrac * b);
        }
    } 

    /* Increment the wavetable/window phase, pulsewidth/mod phase & the morph phase */
    p->wts->phase += isr * p->freq;
    p->wins->phase += isr * p->freq;

    /* Increment the burst phase on pulse boundries */
    if(p->burst != NULL && p->wins->phase >= p->wins->length) {
        p->burst->phase += 1;
    }

    /* Prevent phase overflow by subtracting the boundries if they have been passed */
    if(p->wts->phase >= 1.f) p->wts->phase -= 1.f;
    if(p->wins->phase >= 1.f) p->wins->phase -= 1.f;
    if(p->burst != NULL && p->burst->phase >= p->burst->length) p->burst->phase -= p->burst->length;

    /* Multiply the wavetable value by the window value */
    return sample * mod;
}

void destroy_pulsarosc(lppulsarosc_t* p) {
    LPBuffer.destroy_stack(p->wts);
    LPBuffer.destroy_stack(p->wins);
    LPArray.destroy(p->burst);
    LPMemoryPool.free(p);
}


const lppulsarosc_factory_t LPPulsarOsc = { create_pulsarosc, process_pulsarosc, destroy_pulsarosc };
