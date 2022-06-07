#include "oscs.bln.h"

lpblnosc_t * create_blnosc(lpbuffer_t * buf, lpfloat_t minfreq, lpfloat_t maxfreq);
lpfloat_t process_blnosc(lpblnosc_t * osc);
lpbuffer_t * render_blnosc(lpblnosc_t * osc, size_t length, lpbuffer_t * amp, int channels);
void destroy_blnosc(lpblnosc_t * osc);

const lpblnosc_factory_t LPBLNOsc = { create_blnosc, process_blnosc, render_blnosc, destroy_blnosc };

lpblnosc_t * create_blnosc(lpbuffer_t * buf, lpfloat_t minfreq, lpfloat_t maxfreq) {
    lpblnosc_t* osc = (lpblnosc_t*)LPMemoryPool.alloc(1, sizeof(lpblnosc_t));
    osc->buf = buf;
    osc->samplerate = (lpfloat_t)DEFAULT_SAMPLERATE;
    osc->gate = 0;
    osc->phase = 0.f;
    osc->phaseinc = (1.f / osc->samplerate) * (lpfloat_t)buf->length;
    osc->minfreq = minfreq;
    osc->maxfreq = maxfreq;
    osc->freq = LPRand.rand(osc->minfreq, osc->maxfreq);;

    return osc;
}

lpfloat_t process_blnosc(lpblnosc_t * osc) {
    int c;
    lpfloat_t sample, f, a, b;
    size_t idxa, idxb, boundry;

    boundry = osc->buf->length-1;

    f = osc->phase - (int)osc->phase;
    idxa = (size_t)osc->phase;
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
        osc->freq = LPRand.rand(osc->minfreq, osc->maxfreq);;
    } else {
        osc->gate = 0;
    }

    return sample;
}

lpbuffer_t * render_blnosc(lpblnosc_t * osc, size_t length, lpbuffer_t * amp, int channels) {
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
        sample = process_blnosc(osc);
        for(c=0; c < channels; c++) {
            out->data[i * channels + c] = sample * _amp;
        }
    }

    return out;
}

void destroy_blnosc(lpblnosc_t * osc) {
    LPMemoryPool.free(osc);
}


