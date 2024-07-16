#include "pippi.h"

#define SR 48000
#define CHANNELS 2

int main() {
    size_t i, length;
    lpbuffer_t * out, * src, * win;
    lpformation_t * formation;
    int c, numgrains=6;

    src = LPSoundFile.read("../tests/sounds/living.wav");
    win = LPWindow.create(WIN_HANN, 4096);

    length = 600 * src->samplerate;
    out = LPBuffer.create(length, src->channels, src->samplerate);
    formation = LPFormation.create(numgrains, src, win);

    /* Render each frame of the grainformation */
    for(i=0; i < length; i++) {
        if(LPRand.rand(0,1) > 0.99f) {
            formation->offset += LPRand.rand(0, LPRand.rand(.000005f, .0001f));
            formation->speed = LPRand.randint(0,6)*0.5f+0.125f;
            formation->grainlength = LPRand.rand(0.01f, 0.5f);
        }
        LPFormation.process(formation);

        for(c=0; c < out->channels; c++) {
            out->data[i * out->channels + c] = formation->current_frame->data[c];
        }
    }

    LPFX.norm(out, 0.8f);
    LPSoundFile.write("renders/grainformation-out.wav", out);

    LPBuffer.destroy(out);
    LPBuffer.destroy(src);
    LPBuffer.destroy(win);
    LPFormation.destroy(formation);

    return 0;
}
