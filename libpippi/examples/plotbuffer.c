#include "pippi.h"

int main(void) {
    setlocale(LC_ALL, "");
    lpbuffer_t * out = LPSoundFile.read("../tests/sounds/guitar1s-fadeout.wav");
    LPBuffer.plot(out);
}
