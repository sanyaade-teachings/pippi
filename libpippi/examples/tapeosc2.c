#include "pippi.h"

#define BS 4096
#define SR 48000
#define CHANNELS 2

int main() {
    size_t i, c, length;
    lpbuffer_t * snd;
    lpbuffer_t * out;
    lpbuffer_t * speeds;
    lptapeosc_t * osc;
    lpfloat_t speedphase, speedphaseinc;

    length = 60 * SR;

    snd = LPSoundFile.read("../tests/sounds/living.wav");

    speeds = LPWindow.create(WIN_HANN, BS);
    LPBuffer.scale(speeds, 0, 1, SR/10.f, (float)SR);

    out = LPBuffer.create(length, CHANNELS, SR);
    osc = LPTapeOsc.create(snd, SR);
    osc->samplerate = SR;

    speedphaseinc = 1.f/speeds->length;
    speedphase = 0.f;

    for(i=0; i < length; i++) {
        osc->range = LPInterpolation.linear(speeds, ((float)i/length) * speeds->length);

        LPTapeOsc.process(osc);
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = osc->current_frame->data[c];
        }
        speedphase += speedphaseinc;
        if(speedphase >= speeds->length) {
            speedphase -= speeds->length;
        }
    }

    LPSoundFile.write("renders/tapeosc2-out.wav", out);

    LPTapeOsc.destroy(osc);
    LPBuffer.destroy(out);
    LPBuffer.destroy(snd);
    LPBuffer.destroy(speeds);

    return 0;
}
