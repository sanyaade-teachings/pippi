#include "oscs.tape.h"

lptapeosc_t * create_tapeosc(lpbuffer_t * buf);
void process_tapeosc(lptapeosc_t * osc);
void rewind_tapeosc(lptapeosc_t * osc);
lpbuffer_t * render_tapeosc(lptapeosc_t * osc, size_t length, lpbuffer_t * amp, int channels);
void destroy_tapeosc(lptapeosc_t * osc);

const lptapeosc_factory_t LPTapeOsc = { 
    create_tapeosc, 
    process_tapeosc, 
    rewind_tapeosc, 
    render_tapeosc, 
    destroy_tapeosc 
};

lptapeosc_t * create_tapeosc(lpbuffer_t * buf) {
    assert(buf != NULL);
    assert(buf->channels > 0);
    assert(buf->samplerate > 0);

    lptapeosc_t* osc = (lptapeosc_t*)LPMemoryPool.alloc(1, sizeof(lptapeosc_t));
    osc->buf = buf;

    osc->samplerate = buf->samplerate;
    osc->range = buf->length;
    osc->gate = 0;
    osc->pulsewidth = 1.f;

    osc->phase = 0.f;
    osc->speed = 1.f;
    osc->start = 0.f;
    osc->current_frame = LPBuffer.create(1, buf->channels, buf->samplerate);
    return osc;
}

void rewind_tapeosc(lptapeosc_t * osc) {
    osc->start = 0;
    osc->phase = 0;
}

void process_tapeosc(lptapeosc_t * osc) {
    lpfloat_t sample, f, a, b, phase, ipw=0.f;
    int c, channels;
    size_t idxa, idxb;

    assert(osc->range != 0);

    channels = osc->buf->channels;
    if(osc->pulsewidth > 0 && osc->phase < osc->pulsewidth) {
        ipw = 1.f/osc->pulsewidth;

        phase = osc->phase * (osc->range * ipw) + osc->start;
        while(phase >= osc->buf->length-1) phase -= osc->buf->length-1;

        f = phase - (int)phase;
        idxa = (size_t)phase;
        idxb = idxa + 1;

        for(c=0; c < channels; c++) {
            a = osc->buf->data[idxa * channels + c];
            b = osc->buf->data[idxb * channels + c];
            sample = (1.f - f) * a + (f * b);
            osc->current_frame->data[c] = sample;
        }
    } else {
        memset(osc->current_frame->data, 0, sizeof(lpfloat_t) * channels);
    }

    osc->phase += osc->speed * (1.f/osc->range) * osc->pulsewidth;

    if(osc->phase >= 1.f) {
        osc->gate = 1;
    } else {
        osc->gate = 0;
    }

    while(osc->phase >= 1.f) osc->phase -= 1.f;
}

lpbuffer_t * render_tapeosc(lptapeosc_t * osc, size_t length, lpbuffer_t * amp, int channels) {
    lpbuffer_t * out;
    lpfloat_t _amp;
    size_t i;
    int c;
    float pos;

    pos = 0.f;
    out = LPBuffer.create(length, channels, osc->samplerate);
    for(i=0; i < length; i++) {
        pos = (float)i/length;
        _amp = LPInterpolation.linear_pos(amp, pos);
        process_tapeosc(osc);
        for(c=0; c < channels; c++) {
            out->data[i * channels + c] = osc->current_frame->data[c] * _amp;
        }
    }

    return out;
}

void destroy_tapeosc(lptapeosc_t * osc) {
    LPMemoryPool.free(osc);
}


