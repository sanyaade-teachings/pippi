#include "pippi.h"

int main() {
    lpbuffer_t * a;
    lpbuffer_t * b;
    lpbuffer_t * out;

    a = LPSoundFile.read("../../pippi/tests/sounds/guitar10s.wav");
    b = LPBuffer.cut(a, 100, 1000);

    out = LPSpectral.convolve(a, b);

    LPSoundFile.write("renders/convolution2-out.wav", out);

    LPBuffer.destroy(a);
    LPBuffer.destroy(b);
    LPBuffer.destroy(out);

    return 0;
}
