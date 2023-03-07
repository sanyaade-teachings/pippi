#include "pippi.h"

int main() {
    lpbuffer_t * src;
    lpbuffer_t * out;
    size_t start, length;

    src = LPSoundFile.read("../../pippi/tests/sounds/living.wav");

    length = LPRand.randint(50, 48000);
    start = LPRand.randint(0, src->length - length);

    out = LPBuffer.cut(src, start, length);
    LPBuffer.taper(out, out->length/2, out->length/10);

    LPSoundFile.write("renders/taperbuffer-out.wav", out);

    LPBuffer.destroy(src);
    LPBuffer.destroy(out);

    return 0;
}
