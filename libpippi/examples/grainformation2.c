#include "pippi.h"

#define SR 48000
#define CHANNELS 2

int main() {
    size_t i, c, length, numgrains, mingrainlength;
    lpbuffer_t * snd;
    lpbuffer_t * out;

    lpbuffer_t * skew_wt;
    lpbuffer_t * scrub_wt;
    lpbuffer_t * cutoffs_wt;
    lpbuffer_t * spread_wt;
    lpbuffer_t * maxgrainlength_wt;

    lpshapes_t * skew;
    lpshapes_t * scrub;
    lpshapes_t * cutoffs;
    lpshapes_t * spread;
    lpshapes_t * maxgrainlength;

    lpformation_t * formation;

    lpfloat_t cutoff;
    lpfloat_t ys[CHANNELS];

    length = 10 * SR;
    numgrains = 1000;
    mingrainlength = SR/200.f;
    ys[0] = 0.f;
    ys[1] = 0.f;

    out = LPBuffer.create(length, CHANNELS, SR);
    snd = LPSoundFile.read("../tests/sounds/living.wav");
    formation = LPFormation.create(numgrains, mingrainlength * 100, mingrainlength, length, CHANNELS, SR);

    spread_wt = LPWindow.create(WT_HANN, 4096);
    skew_wt = LPWindow.create(WT_HANN, 4096);
    scrub_wt = LPWindow.create(WT_HANN, 4096);
    cutoffs_wt = LPWindow.create(WT_HANN, 4096);
    maxgrainlength_wt = LPWindow.create(WT_HANN, 4096);

    LPBuffer.scale(spread_wt, 0, 1, 0.f, 1.f);
    LPBuffer.scale(skew_wt, 0, 1, 0.5f, 1.f);
    LPBuffer.scale(scrub_wt, 0, 1, 0.03f, 1.f);
    LPBuffer.scale(cutoffs_wt, 0, 1, 200.f, 1000.f);
    LPBuffer.scale(maxgrainlength_wt, 0, 1, mingrainlength, mingrainlength * 100);

    spread = LPShapes.create(spread_wt);
    skew = LPShapes.create(skew_wt);
    /*skew->maxfreq = 1000.f;*/
    scrub = LPShapes.create(scrub_wt);
    cutoffs = LPShapes.create(cutoffs_wt);
    maxgrainlength = LPShapes.create(maxgrainlength_wt);
    /*skew->maxfreq = 0.2f;*/

    formation->speed = 1.f;

    LPRingBuffer.write(formation->rb, snd);

    /* Render each frame of the grainformation */
    for(i=0; i < length; i++) {
        formation->spread = LPShapes.process(spread);
        formation->skew = 1 - LPShapes.process(skew);
        formation->scrub = LPShapes.process(scrub);
        cutoff = LPShapes.process(cutoffs);
        formation->maxlength = LPShapes.process(maxgrainlength);

        LPFormation.process(formation);

        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = LPFX.lpf1(formation->current_frame->data[c] * 0.2f, &ys[c], cutoff, SR);
        }
    }

    LPSoundFile.write("renders/grainformation2-out.wav", out);

    LPBuffer.destroy(out);
    LPBuffer.destroy(snd);
    LPFormation.destroy(formation);
    LPShapes.destroy(cutoffs);
    LPShapes.destroy(maxgrainlength);

    return 0;
}
