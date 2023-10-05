#include "pippi.h"

#define SR 48000
#define CHANNELS 2

int main() {
    size_t i, c, length;
    lpbuffer_t * snd;
    lpbuffer_t * out;
    lpbuffer_t * pw;
    int window_type;

    /*
    lpmultishapeosc_t * cutoffs;
    */
    lpformation_t * formation;

    /*
    lpfloat_t cutoff;
    lpfloat_t ys[CHANNELS];
    */

    window_type = WIN_HANN;
    length = 10 * SR;

    /*
    ys[0] = 0.f;
    ys[1] = 0.f;
    */

    LPRand.seed(112244);

    out = LPBuffer.create(length, CHANNELS, SR);
    snd = LPSoundFile.read("../tests/sounds/living.wav");

    formation = LPFormation.create(window_type, 1, SR/20.f, length, CHANNELS, SR, NULL);
    formation->speed = 1.f;

    /*
    cutoffs = LPShapeOsc.multi(4, WT_COS, WT_TRI, WT_SINE, WT_SINE);
    cutoffs->min = 20.0f;
    cutoffs->max = 20000.0f;
    cutoffs->maxfreq = 3.f;
    */

    pw = LPWindow.create(WIN_SINE, 4096);
    LPBuffer.scale(pw, 0, 1, 0.f, 0.5f);

    LPRingBuffer.write(formation->rb, snd);

    /* Render each frame of the grainformation */
    for(i=0; i < length; i++) {
        /*formation->pulsewidth = LPInterpolation.linear_pos(pw, (float)i / length);*/
        LPFormation.process(formation);
        /*
        cutoff = LPShapeOsc.multiprocess(cutoffs);
        */

        for(c=0; c < CHANNELS; c++) {
            /*out->data[i * CHANNELS + c] = LPFX.lpf1(formation->current_frame->data[c] * 0.5f, &ys[c], cutoff, SR);*/
            out->data[i * CHANNELS + c] = formation->current_frame->data[c] * 0.5f;
        }
    }

    LPSoundFile.write("renders/grainformation2-out.wav", out);

    LPBuffer.destroy(out);
    LPBuffer.destroy(snd);
    LPFormation.destroy(formation);
    /*
    LPShapeOsc.multidestroy(cutoffs);
    */

    return 0;
}
