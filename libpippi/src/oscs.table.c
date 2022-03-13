#include "oscs.table.h"

lptableosc_t * create_tableosc(lpbuffer_t * buf);
lpfloat_t process_tableosc(lptableosc_t * osc);
lpbuffer_t * render_tableosc(lptableosc_t * osc, size_t length, lpbuffer_t * amp, int channels);
void destroy_tableosc(lptableosc_t * osc);

const lptableosc_factory_t LPTableOsc = { create_tableosc, process_tableosc, render_tableosc, destroy_tableosc };

lptableosc_t * create_tableosc(lpbuffer_t * buf) {
    lptableosc_t* osc = (lptableosc_t*)LPMemoryPool.alloc(1, sizeof(lptableosc_t));
    osc->buf = buf;
    osc->samplerate = (lpfloat_t)DEFAULT_SAMPLERATE;
    osc->gate = 0;
    osc->phase = 0.f;
    osc->phaseinc = 1.f / (lpfloat_t)osc->samplerate;
    osc->freq = 1.f / (lpfloat_t)buf->length;

    return osc;
}

lpfloat_t process_tableosc(lptableosc_t * osc) {
    lpfloat_t sample, f, a, b, phase;
    int c;
    size_t idxa, idxb, boundry;

    phase = osc->phase;
    boundry = osc->buf->length-1;

    f = phase - (int)phase;
    idxa = (size_t)phase;
    idxb = idxa + 1;

    sample = 0.f;

    for(c=0; c < osc->buf->channels; c++) {
        a = osc->buf->data[idxa * osc->buf->channels + c];
        b = osc->buf->data[idxb * osc->buf->channels + c];
        sample += (1.f - f) * a + (f * b);
    }

    osc->phase += osc->phaseinc * osc->freq;

    if(osc->phase >= boundry) {
        osc->phase -= boundry;
        osc->gate = 1;
    } else {
        osc->gate = 0;
    }

    return sample;
}

lpbuffer_t * render_tableosc(lptableosc_t * osc, size_t length, lpbuffer_t * amp, int channels) {
    lpbuffer_t * out;
    lpfloat_t _amp, sample;
    size_t i;
    int c;
    float pos;

    pos = 0.f;
    out = LPBuffer.create(length, channels, osc->samplerate);
    for(i=0; i < length; i++) {
        pos = (float)i/length;
        _amp = LPInterpolation.linear_pos(amp, pos);
        sample = process_tableosc(osc);
        for(c=0; c < channels; c++) {
            out->data[i * channels + c] = sample * _amp;
        }
    }

    return out;
}

void destroy_tableosc(lptableosc_t * osc) {
    LPMemoryPool.free(osc);
}


