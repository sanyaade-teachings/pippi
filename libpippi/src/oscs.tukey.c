#include "pippicore.h"
#include "oscs.tukey.h"

lptukeyosc_t * create_tukeyosc(void);
lpfloat_t process_tukeyosc(lptukeyosc_t * osc);
lpbuffer_t * render_tukeyosc(lptukeyosc_t * osc, size_t length, lpbuffer_t * freq, lpbuffer_t * amp, int channels);
void destroy_tukeyosc(lptukeyosc_t * osc);

const lptukeyosc_factory_t LPTukeyOsc = { create_tukeyosc, process_tukeyosc, render_tukeyosc, destroy_tukeyosc };

lptukeyosc_t * create_tukeyosc(void) {
    lptukeyosc_t * osc = (lptukeyosc_t *)LPMemoryPool.alloc(1, sizeof(lptukeyosc_t));
    osc->phase = 0;
    osc->freq = 220.0;
    osc->shape = 0.5f;
    osc->samplerate = 48000.0;
    osc->direction = 1;
    return osc;
}

lpfloat_t process_tukeyosc(lptukeyosc_t* osc) {
    lpfloat_t pos, a, r, sample, direction;

    if(osc->shape < 0.00001f) osc->shape = 0.00001f;
    if(osc->shape > 1.f) osc->shape = 1.f;

    a = PI2 / osc->shape;

    /* Implementation based on https://www.mathworks.com/help/signal/ref/tukeywin.html */
    if(osc->phase <= r / 2.f) {
        sample = 0.5 * (1.f + cos(a * (osc->phase - r / 2.f)));
    } else if(osc->phase < 1 - (r/2)) {
        sample = 1.f;
    } else {
        sample = 0.5f * (1.f + cos(a * (osc->phase - 1.f + r / 2.f)));
    }

    sample *= osc->direction;

    osc->phase += (1.0/osc->samplerate) * osc->freq * 2.f;

    if(osc->phase > 1.f) {
        osc->direction *= -1;
    }

    while(osc->phase >= 1.f) {
        osc->phase -= 1.f;
    }

    return sample;
}

lpbuffer_t * render_tukeyosc(lptukeyosc_t * osc, size_t length, lpbuffer_t * freq, lpbuffer_t * amp, int channels) {
    lpbuffer_t * out;
    lpfloat_t sample, _amp;
    size_t i, c;
    float pos;

    pos = 0.f;
    out = LPBuffer.create(length, channels, osc->samplerate);
    for(i=0; i < length; i++) {
        pos = (float)i/length;
        osc->freq = LPInterpolation.linear_pos(freq, pos);
        _amp = LPInterpolation.linear_pos(amp, pos);
        sample = process_tukeyosc(osc) * _amp;
        for(c=0; c < channels; c++) {
            out->data[i * channels + c] = sample;
        }
    }

    return out;
}

void destroy_tukeyosc(lptukeyosc_t * osc) {
    LPMemoryPool.free(osc);
}


