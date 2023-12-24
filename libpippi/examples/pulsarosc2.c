#include "pippi.h"

#define CHANNELS 2
#define SR 48000
#define TABLESIZE 128

int main() {
    size_t length = SR * 10;
    size_t i, c;
    lpbuffer_t * wtSine;
    lpbuffer_t * wtTri;
    lpbuffer_t * wt;
    lpbuffer_t * win;

    lppulsarosc_t * osc;
    lpfloat_t freq = 220.f;
    lpfloat_t sample = 0;

    size_t wt_onsets[2] = {0};
    size_t wt_lengths[2] = {0};

    size_t win_onsets[1] = {0};
    size_t win_lengths[1] = {TABLESIZE};

    wtSine = LPWavetable.create(WT_SINE, TABLESIZE);
    wtTri = LPWavetable.create(WT_TRI, TABLESIZE);
    wt = LPBuffer.create(TABLESIZE*2, 1, SR);
    win = LPWindow.create(WIN_HANN, TABLESIZE);

    for(i=0; i < TABLESIZE; i++) {
        wt->data[i] = wtSine->data[i];
    }

    for(i=0; i < TABLESIZE; i++) {
        wt->data[i+TABLESIZE] = wtTri->data[i];
    }

    lpbuffer_t * buf = LPBuffer.create(length, CHANNELS, SR);    

    wt_lengths[0] = TABLESIZE;
    wt_lengths[1] = TABLESIZE;
    wt_onsets[1] = TABLESIZE;
    win_lengths[0] = TABLESIZE;

    osc = LPPulsarOsc.create(
        2, wt->data, wt->length, wt_onsets, wt_lengths, 
        1, win->data, win->length, win_onsets, win_lengths
    );

    osc->samplerate = SR;
    osc->freq = freq;

    for(i=0; i < length; i++) {
        sample = LPPulsarOsc.process(osc);
        for(c=0; c < CHANNELS; c++) {
            buf->data[i * CHANNELS + c] = sample;
        }
    }

    LPSoundFile.write("renders/pulsarosc2-out.wav", buf);

    LPPulsarOsc.destroy(osc);
    LPBuffer.destroy(buf);
    LPBuffer.destroy(wt);
    LPBuffer.destroy(win);

    return 0;
}


