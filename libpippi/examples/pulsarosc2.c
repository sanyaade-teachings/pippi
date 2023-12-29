#include "pippi.h"

#define CHANNELS 2
#define SR 48000
#define TABLESIZE 128

#define NUMWT 3
#define NUMWIN 2
#define WTSIZE 4096

int main() {
    size_t length = SR * 10;
    size_t i, c;
    lpbuffer_t * wt;
    lpbuffer_t * win;

    lppulsarosc_t * osc;
    lpfloat_t freq = 220.f;
    lpfloat_t sample = 0;

    size_t wt_onsets[NUMWT] = {};
    size_t wt_lengths[NUMWT] = {};
    size_t win_onsets[NUMWIN] = {};
    size_t win_lengths[NUMWIN] = {};

    wt = LPWavetable.create_stack(NUMWT, 
            wt_onsets, wt_lengths,
            WT_SINE, WTSIZE,
            WT_RND, WTSIZE,
            WT_TRI, WTSIZE
    );

    win = LPWindow.create_stack(NUMWIN, 
            win_onsets, win_lengths,
            WIN_HANN, WTSIZE,
            WIN_SINE, WTSIZE
    );

    lpbuffer_t * buf = LPBuffer.create(length, CHANNELS, SR);    

    osc = LPPulsarOsc.create(
        NUMWT, wt->data, wt->length, wt_onsets, wt_lengths, 
        NUMWIN, win->data, win->length, win_onsets, win_lengths
    );

    osc->samplerate = SR;
    osc->freq = freq;

    for(i=0; i < length; i++) {
        sample = LPPulsarOsc.process(osc);
        for(c=0; c < CHANNELS; c++) {
            buf->data[i * CHANNELS + c] = sample * 0.1f;
        }
    }

    LPSoundFile.write("renders/pulsarosc2-out.wav", buf);

    LPPulsarOsc.destroy(osc);
    LPBuffer.destroy(buf);
    LPBuffer.destroy(wt);
    LPBuffer.destroy(win);

    return 0;
}


