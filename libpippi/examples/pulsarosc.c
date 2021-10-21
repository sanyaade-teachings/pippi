#include "pippi.h"

#define VOICES 4
#define CHANNELS 2
#define SR 48000

int main() {
    size_t length = SR * 60;
    size_t i, c, v;

    lpfloat_t sample = 0;

    lppulsarosc_t * oscs[VOICES];
    lpfloat_t freqs[VOICES] = {220., 330., 440., 550.};

    lpbuffer_t * buf = LPBuffer.create(length, CHANNELS, SR);    

    for(i=0; i < VOICES; i++) {
        oscs[i] = LPPulsarOsc.create();
        oscs[i]->samplerate = SR;
        oscs[i]->freq = freqs[i];
        oscs[i]->saturation = 1;
        oscs[i]->pulsewidth = LPRand.rand(0.01, 1);
        oscs[i]->wts = LPWavetable.create_stack(4, WT_SINE, WT_SQUARE, WT_TRI, WT_SINE);
        oscs[i]->wins = LPWindow.create_stack(3, WIN_SINE, WIN_HANN, WIN_SINE);
        oscs[i]->burst = LPArray.create_from(4, 1, 1, 0, 1);
    }

    for(i=0; i < length; i++) {
        sample = 0;
        for(v=0; v < VOICES; v++) {
            sample += LPPulsarOsc.process(oscs[v]) * (1.f/VOICES) * 0.99f;
        }

        for(c=0; c < CHANNELS; c++) {
            buf->data[i * CHANNELS + c] = sample;
        }
    }

    LPSoundFile.write("renders/pulsarosc-out.wav", buf);

    for(v=0; v < VOICES; v++) {
        LPPulsarOsc.destroy(oscs[v]);
    }

    LPBuffer.destroy(buf);

    return 0;
}


