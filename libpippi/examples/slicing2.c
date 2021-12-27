#include "pippi.h"

#define NUMCHUNKS 1000

int main() {
    lpbuffer_t * chunks[NUMCHUNKS];
    lpbuffer_t * src;
    lpbuffer_t * out;
    size_t start, length, tlength, pos;
    int i;

    src = LPSoundFile.read("../../pippi/tests/sounds/living.wav");

    tlength = 0;
    for(i=0; i < NUMCHUNKS; i++) {
        length = LPRand.randint(30, 4800);
        start = LPRand.randint(0, src->length - length);
        chunks[i] = LPBuffer.cut(src, start, length);
        tlength += chunks[i]->length;
    }

    out = LPBuffer.create(tlength, src->channels, src->samplerate);

    pos = 0;
    for(i=0; i < NUMCHUNKS; i++) {
        LPBuffer.dub(out, chunks[i], pos);
        pos += chunks[i]->length;
    }

    LPSoundFile.write("renders/slicing2-out.wav", out);

    for(i=0; i < NUMCHUNKS; i++) {
        LPBuffer.destroy(chunks[i]);
    }

    LPBuffer.destroy(out);

    return 0;
}
