#include "oscs.tape.h"

lptapeosc_t * create_tapeosc(lpbuffer_t * buf, lpfloat_t range);
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

lptapeosc_t * create_tapeosc(lpbuffer_t * buf, lpfloat_t range) {
    lpfloat_t samplerate;
    int channels;

    lptapeosc_t* osc = (lptapeosc_t*)LPMemoryPool.alloc(1, sizeof(lptapeosc_t));
    osc->buf = buf;

    if(buf == NULL) {
        samplerate = (lpfloat_t)DEFAULT_SAMPLERATE;
        channels = DEFAULT_CHANNELS;
    } else {
        samplerate = buf->samplerate;
        channels = buf->channels;
    }

    osc->samplerate = samplerate;
    osc->range = range;
    osc->gate = 0;
    osc->pulsewidth = 1.f;

    osc->phase = 0.f;
    osc->speed = 1.f;
    osc->start = 0.f;
    osc->start_increment = range;
    osc->current_frame = LPBuffer.create(1, channels, samplerate);
    return osc;
}

void rewind_tapeosc(lptapeosc_t * osc) {
    osc->start = 0;
    osc->phase = osc->phase - (int)osc->phase;
}

void process_tapeosc(lptapeosc_t * osc) {
    lpfloat_t sample, f, a, b;
    int c, channels;
    size_t idxa, idxb, boundry;

    channels = osc->buf->channels;
    boundry = osc->range + osc->start;

    printf("phase %f channels %i boundry %ld speed %f\n", osc->phase, channels, boundry, osc->speed);

    f = osc->phase - (int)osc->phase;
    idxa = (size_t)osc->phase;
    idxb = idxa + 1;

    for(c=0; c < channels; c++) {
        a = osc->buf->data[idxa * channels + c];
        b = osc->buf->data[idxb * channels + c];
        sample = (1.f - f) * a + (f * b);
        osc->current_frame->data[c] = sample;
    }

    osc->phase += osc->speed;
    printf("phase %f channels %i boundry %ld speed %f\n", osc->phase, channels, boundry, osc->speed);

    if(osc->phase >= boundry) {
        osc->phase = osc->start;
        osc->gate = 1;
    } else {
        osc->gate = 0;
    }
}

/*
void process_tapeosc(lptapeosc_t * osc) {
    lpfloat_t sample, f, a, b, ipw, phase;
    int c, channels;
    size_t idxa, idxb, boundry;

    ipw = 1.f;
    if(osc->pulsewidth > 0) ipw = 1.0f/osc->pulsewidth;

    phase = osc->phase * ipw;
    channels = osc->buf->channels;
    boundry = osc->range + osc->start;

    f = phase - (int)phase;
    idxa = (size_t)phase;
    idxb = idxa + 1;

    if(ipw == 0.f || phase >= osc->buf->length - 1) {
        for(c=0; c < channels; c++) {
            osc->current_frame->data[c] = 0.f;
        }
    } else {
        for(c=0; c < channels; c++) {
            a = osc->buf->data[idxa * channels + c];
            b = osc->buf->data[idxb * channels + c];
            sample = (1.f - f) * a + (f * b);
            osc->current_frame->data[c] = sample;
        }
    }

    osc->phase += osc->speed;

    if(osc->phase >= boundry) {
        osc->phase = osc->start;
        osc->gate = 1;
    } else {
        osc->gate = 0;
    }
}
*/

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


