#include "pippi.h"

int main() {
    lpbuffer_t * a;
    lpbuffer_t * b;
    lpbuffer_t * out;
    int samplerate, channels;
    size_t length;

    samplerate = 48000;
    channels = 2;

    a = LPSoundFile.read("../../pippi/tests/sounds/guitar10s.wav");
    b = LPBuffer.cut(a, 100, 1000);

    length = a->length + b->length + 1;

    out = LPBuffer.create(length, channels, samplerate);

    LPFX.convolve(a, b, out);

    LPSoundFile.write("renders/convolution-out.wav", out);

    LPBuffer.destroy(a);
    LPBuffer.destroy(b);
    LPBuffer.destroy(out);

    return 0;
}
