#include "pippi.h"

int main() {
    lpbuffer_t * out;
    lpbuffer_t * snd;
    lpbuffer_t * speed;
    lpfloat_t pos, sample, warble, phase, wphase, phaseinc, wphaseinc;
    size_t i, length;
    int c;

    snd = LPSoundFile.read("../tests/sounds/living.wav");
    length = snd->length * 2;
    out = LPBuffer.create(length, snd->channels, snd->samplerate);

    speed = LPWindow.create(WIN_SINE, 4096);

    phaseinc = 1.f;
    wphaseinc = 0.8f;

    phase = 0.f;
    wphase = 0.f;

    for(i=0; i < length; i++) {
        pos = (lpfloat_t)i/length;
        for(c=0; c < snd->channels; c++) {
            warble = LPInterpolation.interpolate_linear_channel(snd, wphase, c);
            sample = LPInterpolation.interpolate_linear_channel(snd, phase, c);
            out->data[i * snd->channels + c] = warble + sample;
        }
        phase += phaseinc;
        wphase += LPInterpolation.linear_pos(speed, pos) * 0.01f + 1.f;
    }

    if(wphaseinc > 0) {
        printf("yep\n");
    }

    LPSoundFile.write("renders/warble-out.wav", out);

    return 0;
}
