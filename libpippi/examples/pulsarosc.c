#include "pippi.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#define VOICES 4
#define CHANNELS 2
#define SR 48000
#define num_wts 10
#define num_wins 1
#define TABLESIZE 128

/**
 * 1 open file
 * 2 read N bytes from file (N=sizeof char / burstsize + 1)
 *   into tmp char buffer.
 * 3 loop over each char value
 * 4 loop over each bit value
 *   store bits as bools
 *
 *   in: char buffer?
 *      bool * burst_bytes(unsigned char * bytes)
 *      bool * burst_file(char * filename)
 *
 * */

int main() {
    size_t length = SR * 10;
    size_t i, c, v;
    lpbuffer_t * wts;
    lpbuffer_t * win;

    size_t wt_onsets[num_wts] = {};
    size_t wt_lengths[num_wts] = {};

    size_t win_onsets[num_wins] = {};
    size_t win_lengths[num_wins] = {};

    lpfloat_t sample = 0;

    lppulsarosc_t * oscs[VOICES];
    lpfloat_t freqs[VOICES] = {220., 330., 440., 550.};

    wts = LPWavetable.create_stack(num_wts, 
            wt_onsets, wt_lengths,
            WT_SINE, 4096,
            WT_RND, 4096,
            WT_RND, 4096,
            WT_RND, 4096,
            WT_RND, 4096,
            WT_RND, 4096,
            WT_RND, 4096,
            WT_RND, 4096,
            WT_RND, 4096,
            WT_SINE, 4096
    );

    LPSoundFile.write("renders/pulsarwts.wav", wts);

    win = LPWindow.create_stack(num_wins, 
            win_onsets, win_lengths,
            WIN_HANN, 4096
    );

    LPSoundFile.write("renders/pulsarwin.wav", win);

    lpbuffer_t * buf = LPBuffer.create(length, CHANNELS, SR);    

    for(i=0; i < VOICES; i++) {
        oscs[i] = LPPulsarOsc.create(num_wts, wts->data, wts->length, wt_onsets, wt_lengths, num_wins, win->data, win->length, win_onsets, win_lengths);
        oscs[i]->samplerate = SR;
        oscs[i]->freq = freqs[i];
        oscs[i]->saturation = 1;
        oscs[i]->pulsewidth = LPRand.rand(0.01, 1);
        LPPulsarOsc.burst_file(oscs[i], "examples/kcore.raw", 4096);
    }

    for(i=0; i < length; i++) {
        sample = 0;
        for(v=0; v < VOICES; v++) {
            sample += LPPulsarOsc.process(oscs[v]) * 0.5f;
        }

        for(c=0; c < CHANNELS; c++) {
            buf->data[i * CHANNELS + c] = sample;
        }
    }

    printf("buf length %d channels %d\n", (int)buf->length, buf->channels);
    LPSoundFile.write("renders/pulsarosc-out.wav", buf);

    for(v=0; v < VOICES; v++) {
        LPPulsarOsc.destroy(oscs[v]);
    }

    LPBuffer.destroy(buf);
    LPBuffer.destroy(wts);
    LPBuffer.destroy(win);

    return 0;
}


