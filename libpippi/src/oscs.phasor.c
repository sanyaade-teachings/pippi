#include "pippicore.h"
#include "oscs.phasor.h"

lpphasorosc_t * create_phasorosc(void);
lpfloat_t process_phasorosc(lpphasorosc_t * osc);
lpbuffer_t * render_phasorosc(lpphasorosc_t * osc, size_t length, lpbuffer_t * freq, lpbuffer_t * amp, int channels);
void destroy_phasorosc(lpphasorosc_t * osc);

const lpphasorosc_factory_t LPPhasorOsc = { create_phasorosc, process_phasorosc, render_phasorosc, destroy_phasorosc };

lpphasorosc_t * create_phasorosc(void) {
    lpphasorosc_t * osc = (lpphasorosc_t *)LPMemoryPool.alloc(1, sizeof(lpphasorosc_t));
    osc->phase = 0.f;
    osc->freq = 220.0f;
    osc->samplerate = 48000.0f;
    return osc;
}

lpfloat_t process_phasorosc(lpphasorosc_t* osc) {
    osc->phase += osc->freq * (1.0f/osc->samplerate);
    while(osc->phase >= 1) osc->phase -= 1.0f;
    return osc->phase * 2.f - 1.f;
}

lpbuffer_t * render_phasorosc(lpphasorosc_t * osc, size_t length, lpbuffer_t * freq, lpbuffer_t * amp, int channels) {
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
        sample = process_phasorosc(osc) * _amp;
        for(c=0; c < channels; c++) {
            out->data[i * channels + c] = sample;
        }
    }

    return out;
}

void destroy_phasorosc(lpphasorosc_t * osc) {
    LPMemoryPool.free(osc);
}


