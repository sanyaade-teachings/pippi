#include "pippi.h"

#define SR 48000
#define CHANNELS 2

int main() {
    lpmultishapeosc_t * lfo1;
    lpshapeosc_t * lfo2;
    lpbuffer_t * out;
    lpfloat_t sample;
    int c;
    size_t i=0, length=SR*10;

    out = LPBuffer.create(length, CHANNELS, SR);

    lfo1 = LPShapeOsc.multi(4, WT_COS, WT_TRI, WT_SINE, WT_SINE);
    lfo1->maxfreq = 3.f;

    for(i=0; i < length; i++) {
        sample = LPShapeOsc.multiprocess(lfo1);
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = sample;
        }
    }

    LPSoundFile.write("renders/shapeosc-multi-out.wav", out);

    lfo2 = LPShapeOsc.create(LPWindow.create(WT_SINE, 4096));
    for(i=0; i < length; i++) {
        sample = LPShapeOsc.process(lfo2);
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = sample;
        }
    }

    LPSoundFile.write("renders/shapeosc-out.wav", out);

    return 0;
}
