#include "pippicore.h"
#include "oscs.fract.h"

lpfractosc_t * create_fractosc(void);
lpfloat_t process_fractosc(lpfractosc_t * osc);
lpbuffer_t * render_fractosc(lpfractosc_t * osc, size_t length, lpbuffer_t * freq, lpbuffer_t * amp, int channels);
void destroy_fractosc(lpfractosc_t * osc);

const lpfractosc_factory_t LPFractOsc = { create_fractosc, process_fractosc, render_fractosc, destroy_fractosc };

lpfractosc_t * create_fractosc(void) {
    lpfractosc_t * osc = (lpfractosc_t *)LPMemoryPool.alloc(1, sizeof(lpfractosc_t));
    osc->phase = 0.f;
    osc->freq = 220.0f;
    osc->depth = 0;
    osc->samplerate = (lpfloat_t)DEFAULT_SAMPLERATE;
    return osc;
}

lpfloat_t process_fractosc(lpfractosc_t* osc) {
    lpfloat_t sample, depth;
    depth = fmin(osc->depth, 9);
    sample = sin((lpfloat_t)PI2 * osc->phase);
    sample *= powf(10.f, depth);
    sample = sample - (int)sample;

    printf("sample %f depth %d pow %f\n", (float)sample, (int)osc->depth, (float)pow(10.f, osc->depth));

    assert(sample <= 1.f && sample >= -1.f);

    osc->phase += osc->freq * (1.0f/osc->samplerate);

    while(osc->phase >= 1) {
        osc->phase -= 1.0f;
    }

    return sample;
}

lpbuffer_t * render_fractosc(lpfractosc_t * osc, size_t length, lpbuffer_t * freq, lpbuffer_t * amp, int channels) {
    lpbuffer_t * out;
    lpfloat_t sample, _amp;
    size_t i;
    int c;
    float pos;

    pos = 0.f;
    out = LPBuffer.create(length, channels, osc->samplerate);
    for(i=0; i < length; i++) {
        pos = (float)i/length;
        osc->freq = LPInterpolation.linear_pos(freq, pos);
        _amp = LPInterpolation.linear_pos(amp, pos);
        sample = process_fractosc(osc) * _amp;
        for(c=0; c < channels; c++) {
            out->data[i * channels + c] = sample;
        }
    }

    return out;
}

void destroy_fractosc(lpfractosc_t * osc) {
    LPMemoryPool.free(osc);
}


