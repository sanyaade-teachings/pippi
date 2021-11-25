#include "pippi.h"

#define SR 48000
#define CHANNELS 2

int main() {
    size_t i, c, length, numgrains, mingrainlength;
    lpbuffer_t * snd;
    lpbuffer_t * out;
    lpbuffer_t * skew;
    lpbuffer_t * scrub;
    lpbuffer_t * cutoffs;
    lpbuffer_t * spread;
    lpbuffer_t * maxgrainlength;
    lpformation_t * formation;
    lpfloat_t pos, cutoff;
    lpfloat_t ys[CHANNELS];

    length = 10 * SR;
    numgrains = 100;
    mingrainlength = SR/200.f;
    ys[0] = 0.f;
    ys[1] = 0.f;

    out = LPBuffer.create(length, CHANNELS, SR);
    snd = LPSoundFile.read("../tests/sounds/living.wav");
    formation = LPFormation.create(numgrains, mingrainlength * 100, mingrainlength, length, CHANNELS, SR);

    spread = LPWindow.create(WT_HANN, 4096);
    skew = LPWindow.create(WT_HANN, 4096);
    scrub = LPWindow.create(WT_HANN, 4096);
    cutoffs = LPWindow.create(WT_TRI, 4096);
    maxgrainlength = LPWindow.create(WT_HANN, 4096);

    LPBuffer.scale(spread, 0, 1, 0.f, 1.f);
    LPBuffer.scale(cutoffs, 0, 1, 20.f, 1000.f);
    LPBuffer.scale(skew, 0, 1, 0.f, 1.f);
    LPBuffer.scale(scrub, 0, 1, 0.03f, 0.2f);
    LPBuffer.scale(maxgrainlength, 0, 1, mingrainlength, mingrainlength * 100);

    formation->speed = 1.f;

    LPRingBuffer.write(formation->rb, snd);

    /* Render each frame of the grainformation */
    for(i=0; i < length; i++) {
        pos = (float)i / length;
        formation->spread = LPInterpolation.linear_pos(spread, pos);
        formation->skew = 1 - LPInterpolation.linear_pos(skew, pos);
        formation->scrub = LPInterpolation.linear_pos(scrub, pos);
        formation->maxlength = LPInterpolation.linear_pos(maxgrainlength, pos);
        cutoff = LPInterpolation.linear_pos(cutoffs, pos);

        LPFormation.process(formation);

        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = LPFX.lpf1(formation->current_frame->data[c] * 0.2f, &ys[c], cutoff, SR);
        }
    }

    LPSoundFile.write("renders/grainformation2-out.wav", out);

    LPBuffer.destroy(out);
    LPBuffer.destroy(snd);
    LPFormation.destroy(formation);

    return 0;
}
