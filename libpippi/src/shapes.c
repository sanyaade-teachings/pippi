#include "shapes.h"

lpfloat_t shapes_process(lpshapes_t * s) {
    lpfloat_t out, j, d, isamplerate, freqwidth;

    isamplerate = 1.0f/s->samplerate;
    freqwidth = s->maxfreq - s->minfreq;

    s->freq = (lpfloat_t)fmax((s->density * freqwidth) + s->minfreq + LPRand.rand(freqwidth * -s->periodicity, freqwidth * s->periodicity), s->minfreq);

    out = LPInterpolation.linear(s->wt, s->phase);

    s->phase += isamplerate * (s->wt->length-1) * s->freq;
    j = (lpfloat_t)log(s->stability * ((lpfloat_t)EULER-1.f) + 1.f);
    if(s->phase > s->wt->length && j > LPRand.rand(0.f,1.f)) {
        d = log(s->density * ((lpfloat_t)EULER-1) + 1);
        s->freq = (lpfloat_t)fmax((d * freqwidth) + s->minfreq + LPRand.rand(freqwidth * -s->periodicity, freqwidth * s->periodicity), s->minfreq);
    }

    while(s->phase > s->wt->length) {
        s->phase -= s->wt->length;
    }

    return out;
}

lpshapes_t * shapes_create(lpbuffer_t * wt) {
    lpshapes_t * s;

    s = (lpshapes_t *)LPMemoryPool.alloc(1, sizeof(lpshapes_t));
    s->wt = wt;

    s->density = 0.5f;
    s->periodicity = 0.5f;
    s->stability = 0.25f;

    s->phase = 0.f;
    s->freq = 1.f;
    s->minfreq = 0.05f;
    s->maxfreq = 900.f;
    s->samplerate = DEFAULT_SAMPLERATE;

    return s;
}

void shapes_destroy(lpshapes_t * s) {
    LPMemoryPool.free(s); 
}

const lpshapes_factory_t LPShapes = { shapes_create, shapes_process, shapes_destroy };
