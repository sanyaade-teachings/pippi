#include "pippi.h"

int main() {
    lpbuffer_t * wt;

    wt = LPWavetable.create(WT_SINE, 4096);
    LPSoundFile.write("renders/wavetable-sine.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWavetable.create(WT_COS, 4096);
    LPSoundFile.write("renders/wavetable-cosine.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWavetable.create(WT_TRI, 4096);
    LPSoundFile.write("renders/wavetable-tri.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWavetable.create(WT_TRI2, 4096);
    LPSoundFile.write("renders/wavetable-tri2.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWavetable.create(WT_SQUARE, 4096);
    LPSoundFile.write("renders/wavetable-square.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWavetable.create(WT_SAW, 4096);
    LPSoundFile.write("renders/wavetable-saw.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWavetable.create(WT_RSAW, 4096);
    LPSoundFile.write("renders/wavetable-rsaw.wav", wt);
    LPBuffer.destroy(wt);

    return 0;
}
