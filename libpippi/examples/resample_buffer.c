#include "pippi.h"

int main() {
    lpbuffer_t * snd;
    lpbuffer_t * snd2;

    snd = LPSoundFile.read("../tests/sounds/living.wav");
    snd2 = LPBuffer.resample(snd, snd->length * 2);
    LPSoundFile.write("renders/resample_buffer-out.wav", snd2);

    LPBuffer.destroy(snd);
    LPBuffer.destroy(snd2);

    return 0;
}
