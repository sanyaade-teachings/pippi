#include "pippi.h"

int main() {
    lpbuffer_t * a;
    lpbuffer_t * b;
    lpbuffer_t * c;
    lpbuffer_t * d;
    lpbuffer_t * e;

    lpbuffer_t * out;
    size_t start, length, tlength, pos;

    a = LPSoundFile.read("../../pippi/tests/sounds/living.wav");

    tlength = 0;
    pos = 0;
    printf("a length %d\n", (int)a->length);

    length = LPRand.randint(50, 48000);
    start = LPRand.randint(0, a->length - length);
    b = LPBuffer.cut(a, start, length);
    tlength += b->length;

    length = LPRand.randint(50, 48000);
    start = LPRand.randint(0, a->length - length);
    c = LPBuffer.cut(a, start, length);
    tlength += c->length;

    length = LPRand.randint(50, 48000);
    start = LPRand.randint(0, a->length - length);
    d = LPBuffer.cut(a, start, length);
    tlength += d->length;

    length = LPRand.randint(50, 48000);
    start = LPRand.randint(0, a->length - length);
    e = LPBuffer.cut(a, start, length);
    tlength += e->length;

    out = LPBuffer.create(tlength, a->channels, a->samplerate);

    LPBuffer.dub(out, b, pos);
    pos += b->length;

    LPBuffer.dub(out, c, pos);
    pos += c->length;

    LPBuffer.dub(out, d, pos);
    pos += d->length;

    LPBuffer.dub(out, e, pos);

    LPSoundFile.write("renders/slicing-out.wav", out);

    LPBuffer.destroy(a);
    LPBuffer.destroy(b);
    LPBuffer.destroy(c);
    LPBuffer.destroy(d);
    LPBuffer.destroy(e);
    LPBuffer.destroy(out);

    return 0;
}
