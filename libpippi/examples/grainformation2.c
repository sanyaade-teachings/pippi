#include "pippi.h"

#define SR 48000
#define CHANNELS 2

int main() {
    size_t i, c, length, numgrains, maxgrainlength, mingrainlength;
    lpbuffer_t * snd;
    lpbuffer_t * out;
    lpbuffer_t * skew;
    lpbuffer_t * scrub;
    lpformation_t * formation;
    lpfloat_t pos;

    length = 60 * SR;
    numgrains = 100;
    maxgrainlength = SR/1.f;
    mingrainlength = SR/200.f;

    out = LPBuffer.create(length, CHANNELS, SR);
    snd = LPSoundFile.read("../tests/sounds/living.wav");
    formation = LPFormation.create(numgrains, maxgrainlength, mingrainlength, length, CHANNELS, SR);
    skew = LPWindow.create(WT_HANN, 4096);
    scrub = LPWindow.create(WT_HANN, 4096);
    LPBuffer.scale(skew, 0, 1, 0.f, 1.f);
    LPBuffer.scale(scrub, 0, 1, 0.03f, 100.f);

    formation->spread = 1;
    formation->speed = 1.f;

    LPRingBuffer.write(formation->rb, snd);

    /* Render each frame of the grainformation */
    for(i=0; i < length; i++) {
        pos = (float)i / length;
        formation->skew = 1 - LPInterpolation.linear_pos(skew, pos);
        formation->scrub = LPInterpolation.linear_pos(scrub, pos);

        LPFormation.process(formation);

        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = formation->current_frame->data[c] * 0.1f;
        }
    }

    LPSoundFile.write("renders/grainformation2-out.wav", out);

    LPBuffer.destroy(out);
    LPBuffer.destroy(snd);
    LPFormation.destroy(formation);

    return 0;
}
