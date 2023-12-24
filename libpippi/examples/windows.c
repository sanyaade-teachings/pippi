#include "pippi.h"

int main() {
    lpbuffer_t * wt;

    wt = LPWindow.create(WIN_SINE, 4096);
    LPSoundFile.write("renders/window-sine.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWindow.create(WIN_SINEIN, 4096);
    LPSoundFile.write("renders/window-sinein.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWindow.create(WIN_SINEOUT, 4096);
    LPSoundFile.write("renders/window-sineout.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWindow.create(WIN_COS, 4096);
    LPSoundFile.write("renders/window-cos.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWindow.create(WIN_TRI, 4096);
    LPSoundFile.write("renders/window-tri.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWindow.create(WIN_PHASOR, 4096);
    LPSoundFile.write("renders/window-phasor.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWindow.create(WIN_HANN, 4096);
    LPSoundFile.write("renders/window-hann.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWindow.create(WIN_SAW, 4096);
    LPSoundFile.write("renders/window-saw.wav", wt);
    LPBuffer.destroy(wt);

    wt = LPWindow.create(WIN_RSAW, 4096);
    LPSoundFile.write("renders/window-rsaw.wav", wt);
    LPBuffer.destroy(wt);

    return 0;
}
