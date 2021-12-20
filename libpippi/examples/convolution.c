#include "pippi.h"

int main() {
    lpbuffer_t * a;
    lpbuffer_t * b;
    lpbuffer_t * out;
    int samplerate, channels;
    size_t length;

    samplerate = 48000;
    channels = 2;

    a = LPSoundFile.read("../../pippi/tests/sounds/living.wav");
    b = LPSoundFile.read("../../pippi/tests/sounds/guitar1s.wav");

    length = a->length + b->length + 1;

    out = LPBuffer.create(length, channels, samplerate);

    LPFX.convolve(a, b, out);

    LPSoundFile.write("renders/convolution-out.wav", out);

    LPBuffer.destroy(a);
    LPBuffer.destroy(b);
    LPBuffer.destroy(out);

    return 0;
}
