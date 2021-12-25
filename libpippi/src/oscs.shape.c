#include "oscs.shape.h"

lpfloat_t shapeosc_process(lpshapeosc_t * s) {
    lpfloat_t out, j, d, isamplerate, freqwidth, wtdiff, diff;

    isamplerate = 1.0f/s->samplerate;
    freqwidth = s->maxfreq - s->minfreq;

    s->freq = (lpfloat_t)lpfmax((s->density * freqwidth) + s->minfreq + LPRand.rand(freqwidth * -s->periodicity, freqwidth * s->periodicity), s->minfreq);

    out = LPInterpolation.linear(s->wt, s->phase);

    diff = s->max - s->min;
    wtdiff = s->wtmax - s->wtmin;
    out = ((out - s->wtmin) / wtdiff) * diff + s->min;

    s->phase += isamplerate * (s->wt->length-1) * s->freq;
    j = (lpfloat_t)log(s->stability * ((lpfloat_t)EULER-1.f) + 1.f);
    if(s->phase > s->wt->length && j > LPRand.rand(0.f,1.f)) {
        d = log(s->density * ((lpfloat_t)EULER-1) + 1);
        s->freq = (lpfloat_t)lpfmax((d * freqwidth) + s->minfreq + LPRand.rand(freqwidth * -s->periodicity, freqwidth * s->periodicity), s->minfreq);
    }

    while(s->phase > s->wt->length) {
        s->phase -= s->wt->length;
    }

    return out;
}

lpfloat_t shapeosc_multiprocess(lpmultishapeosc_t * m) {
    lpfloat_t out;
    int i;
    out = 1.f;
    for(i=0; i < m->numshapeosc; i++) {
        m->shapeosc[i]->density = m->density;
        m->shapeosc[i]->periodicity = m->periodicity;
        m->shapeosc[i]->stability = m->stability;
        m->shapeosc[i]->minfreq = m->minfreq;
        m->shapeosc[i]->maxfreq = m->maxfreq;
        m->shapeosc[i]->samplerate = m->samplerate;

        out *= shapeosc_process(m->shapeosc[i]);
    }

    return out * (m->max - m->min) + m->min;
}

lpshapeosc_t * shapeosc_create(lpbuffer_t * wt) {
    lpshapeosc_t * s;

    s = (lpshapeosc_t *)LPMemoryPool.alloc(1, sizeof(lpshapeosc_t));
    s->wt = wt;

    s->density = 0.5f;
    s->periodicity = 0.5f;
    s->stability = 0.5f;

    s->phase = 0.f;
    s->freq = 10.5f;
    s->minfreq = 0.1f;
    s->maxfreq = 10.5f;
    s->samplerate = DEFAULT_SAMPLERATE;

    s->wtmin = LPBuffer.min(wt);
    s->wtmax = LPBuffer.max(wt);

    s->min = s->wtmin;
    s->max = s->wtmax;

    return s;
}

lpmultishapeosc_t * shapeosc_multicreate(int numshapeosc, ...) {
    lpmultishapeosc_t * m;
    va_list vl;
    int i;

    va_start(vl, numshapeosc);

    m = (lpmultishapeosc_t *)LPMemoryPool.alloc(1, sizeof(lpmultishapeosc_t));
    m->shapeosc = (lpshapeosc_t **)LPMemoryPool.alloc(numshapeosc, sizeof(lpshapeosc_t *));
    for(i=0; i < numshapeosc; i++) {
        m->shapeosc[i] = shapeosc_create(LPWindow.create(va_arg(vl, int), 4096));
    }

    va_end(vl);

    return m;
}

void shapeosc_destroy(lpshapeosc_t * s) {
    LPMemoryPool.free(s); 
}

void shapeosc_multidestroy(lpmultishapeosc_t * m) {
    int i;
    for(i=0; i < m->numshapeosc; i++) {
        LPBuffer.destroy(m->shapeosc[i]->wt); 
        shapeosc_destroy(m->shapeosc[i]); 
    }
    LPMemoryPool.free(m); 
}

const lpshapeosc_factory_t LPShapeOsc = { shapeosc_create, shapeosc_multicreate, shapeosc_process, shapeosc_multiprocess, shapeosc_destroy, shapeosc_multidestroy };
