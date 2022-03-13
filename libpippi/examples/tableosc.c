#include "pippi.h"

#define BS 4096
#define SR 48000
#define CHANNELS 2

int main() {
    size_t i, c, length;
    lpbuffer_t * out;
    lpbuffer_t * win;
    lptableosc_t * osc;
    lpfloat_t sample;

    length = 10 * SR;

    win = LPWindow.create(WIN_RSAW, 512);
    out = LPBuffer.create(length, CHANNELS, SR);

    osc = LPTableOsc.create(win);
    osc->samplerate = SR;
    osc->freq = 110.f;

    for(i=0; i < length; i++) {
        sample = LPTableOsc.process(osc);
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = sample;
        }
    }

    LPSoundFile.write("renders/tableosc-out.wav", out);

    LPTableOsc.destroy(osc);
    LPBuffer.destroy(out);
    LPBuffer.destroy(win);

    return 0;
}
