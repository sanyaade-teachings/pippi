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

lpshapes_t * shapes_create_win(int base) {
    lpshapes_t * s;

    s = (lpshapes_t *)LPMemoryPool.alloc(1, sizeof(lpshapes_t));
    s->wt = LPWindow.create(base, 4096);

    s->density = 0.5f;
    s->periodicity = 0.5f;
    s->stability = 0.5f;

    s->phase = 0.f;
    s->freq = 12.f;
    s->minfreq = 0.3f;
    s->maxfreq = 12.f;
    s->samplerate = DEFAULT_SAMPLERATE;

    return s;
}

lpshapes_t * shapes_create_wt(int base) {
    lpshapes_t * s;

    s = (lpshapes_t *)LPMemoryPool.alloc(1, sizeof(lpshapes_t));
    s->wt = LPWavetable.create(base, 4096);

    s->density = 0.5f;
    s->periodicity = 0.5f;
    s->stability = 0.5f;

    s->phase = 0.f;
    s->freq = 12.f;
    s->minfreq = 0.3f;
    s->maxfreq = 12.f;
    s->samplerate = DEFAULT_SAMPLERATE;

    return s;
}

void shapes_destroy(lpshapes_t * s) {
    LPMemoryPool.free(s); 
}

const lpshapes_factory_t LPShapes = { shapes_create_win, shapes_create_wt, shapes_process, shapes_destroy };
