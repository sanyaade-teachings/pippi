#include "pippi.h"

int main() {
    lpbuffer_t * snd;
    lpbuffer_t * out;
    lpfloat_t bits;
    lpsineosc_t * osc;
    lpfloat_t sample;

    size_t i;
    int c;

    snd = LPSoundFile.read("../tests/sounds/living.wav");
    out = LPBuffer.create(snd->length, snd->channels, snd->samplerate);

    osc = LPSineOsc.create();
    osc->samplerate = snd->samplerate;
    osc->freq = 0.03f;

    for(i=0; i < snd->length; i++) {
        for(c=0; c < snd->channels; c++) {

            // LFO sweep from 2 to 12 bits
            bits = LPSineOsc.process(osc);
            bits += 1.f;
            bits *= 0.5f;
            bits *= 10;
            bits += 2;

            sample = snd->data[i * snd->channels + c];
            sample = LPFX.crush(sample, (int)bits);

            out->data[i * out->channels + c] = sample;
        }
    }

    LPSoundFile.write("renders/crush-out.wav", out);
    LPBuffer.destroy(snd);
    LPBuffer.destroy(out);
    LPSineOsc.destroy(osc);

    return 0;
}
