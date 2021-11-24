#include "oscs.tape.h"

lptapeosc_t * create_tapeosc(lpbuffer_t * buf, lpfloat_t range);
void process_tapeosc(lptapeosc_t * osc);
lpbuffer_t * render_tapeosc(lptapeosc_t * osc, size_t length, lpbuffer_t * amp, int channels);
void destroy_tapeosc(lptapeosc_t * osc);

const lptapeosc_factory_t LPTapeOsc = { create_tapeosc, process_tapeosc, render_tapeosc, destroy_tapeosc };

lptapeosc_t * create_tapeosc(lpbuffer_t * buf, lpfloat_t range) {
    lptapeosc_t* osc = (lptapeosc_t*)LPMemoryPool.alloc(1, sizeof(lptapeosc_t));
    osc->buf = buf;
    osc->samplerate = buf->samplerate;
    osc->range = range;
    osc->gate = 0;

    osc->phase = 0.f;
    osc->speed = 1.f;
    osc->offset = 0.f;
    osc->current_frame = LPBuffer.create(1, buf->channels, buf->samplerate);
    return osc;
}

void process_tapeosc(lptapeosc_t * osc) {
    lpfloat_t sample, pos, f, a, b;
    int c, channels;
    size_t idxa, idxb;

    pos = osc->phase + osc->offset - 1;
    channels = osc->buf->channels;

    idxa = (size_t)pos % osc->buf->length;
    idxb = (size_t)(pos+1) % osc->buf->length;
    f = osc->phase - (int)osc->phase;

    for(c=0; c < channels; c++) {
        a = osc->buf->data[idxa * channels + c];
        b = osc->buf->data[idxb * channels + c];
        sample = (1.f - f) * a + (f * b);
        osc->current_frame->data[c] = sample;
    }

    osc->phase += osc->speed;

    if(osc->phase >= osc->range) {
        osc->phase -= osc->range;
        osc->gate = 1;
    } else {
        osc->gate = 0;
    }
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


