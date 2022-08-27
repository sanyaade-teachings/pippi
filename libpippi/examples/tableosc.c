#include "pippi.h"

#define BS 4096
#define SR 48000
#define CHANNELS 2

int main() {
    size_t i, c, length;
    lpbuffer_t * out;
    lpbuffer_t * win;
    lpbuffer_t * wt;
    lptableosc_t * osc;
    lpfloat_t sample, pos;

    length = 1 * SR;

    win = LPWindow.create(WIN_COS, 512);
    wt = LPWavetable.create(WT_SINE, 512);
    out = LPBuffer.create(length, CHANNELS, SR);

    osc = LPTableOsc.create(wt);
    osc->samplerate = SR;
    osc->freq = 110.f;

    for(i=0; i < length; i++) {
        pos = (lpfloat_t)i / length;
        sample = LPTableOsc.process(osc) * LPInterpolation.linear_pos(win, pos);
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = sample * 0.2f;
        }
    }

    LPSoundFile.write("renders/tableosc-out.wav", out);

    LPTableOsc.destroy(osc);
    LPBuffer.destroy(out);
    LPBuffer.destroy(win);

    return 0;
}
