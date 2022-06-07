#include "pippi.h"

#define BS 4096
#define SR 48000
#define CHANNELS 2

int main() {
    size_t i, c, length;
    lpbuffer_t * out;
    lpbuffer_t * wt;
    lpblnosc_t * osc;
    lpfloat_t sample;

    length = 10 * SR;

    wt = LPWavetable.create(WT_SINE, 512);
    out = LPBuffer.create(length, CHANNELS, SR);

    osc = LPBLNOsc.create(wt, 100.f, 1000.f);
    osc->samplerate = SR;

    for(i=0; i < length; i++) {
        sample = LPBLNOsc.process(osc);
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = sample * 0.2f;
        }
    }

    LPSoundFile.write("renders/blnosc-out.wav", out);

    LPBLNOsc.destroy(osc);
    LPBuffer.destroy(out);
    LPBuffer.destroy(wt);

    return 0;
}
