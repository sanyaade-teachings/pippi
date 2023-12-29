#include "daisy_patch.h"
#include <string>

extern "C" {
#include "../../../libpippi/src/pippicore.h"
#include "../../../libpippi/src/oscs.pulsar.h"
}

/* memory pool size = 64MB */
#define POOLSIZE 67108864
#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128

#define NUMTRIGS 2
#define NUMOSCS 2
#define NUMWINS 1
#define NUMWTS 3

#define WTSIZE 512
#define RINGBUF_SEG_SIZE 1024

using namespace daisy;

unsigned char DSY_SDRAM_BSS pool[POOLSIZE];
lppulsarosc_t * oscs[NUMOSCS];
size_t ringbuf_pos=0, tick=0, seg_region=RINGBUF_SEG_SIZE;
bool trigger_out = false;
int x=0, y=0;

lpbuffer_t * ringbuf;
size_t wt_onsets[NUMWTS] = {0};
size_t wt_lengths[NUMWTS] = {0};
size_t win_onsets[NUMWINS] = {0};
size_t win_lengths[NUMWINS] = {0};

DaisyPatch patch;

void process_encoder() {
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();
    if(patch.encoder.RisingEdge()) {
        tick = 0;
    }
}

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size) {
    float insamp = 0.f;
    for(size_t n = 0; n < size; n++) {
        insamp = in[0][n] * 10.f;
        ringbuf->data[ringbuf_pos] = insamp;
        ringbuf_pos += 1;
        while(ringbuf_pos >= ringbuf->length) ringbuf_pos -= ringbuf->length;

        for(int o=0; o < NUMOSCS; o++) {
            out[o][n] = LPPulsarOsc.process(oscs[o]);
        }
    }
}


int main(void) {
    lpbuffer_t * win;

    LPMemoryPool.init((unsigned char *)pool, POOLSIZE);

    patch.Init();
    patch.DelayMs(100);
    int SR = (int)patch.AudioSampleRate();

    //patch.display.SetCursor(0, 0);
    //std::string str = "Bells";
    //char*       cstr = &str[0];
    //patch.display.WriteString(cstr, Font_7x10, true);

    //patch.display.SetCursor(0, 20);
    //str = std::to_string(SR);
    //patch.display.WriteString(cstr, Font_6x8, true);

    ringbuf = LPWavetable.create_stack(NUMWTS, 
            wt_onsets, wt_lengths,
            WIN_RND, RINGBUF_SEG_SIZE,
            WIN_RND, RINGBUF_SEG_SIZE,
            WIN_RND, RINGBUF_SEG_SIZE
    );

    win = LPWindow.create_stack(NUMWINS, 
            win_onsets, win_lengths,
            WIN_HANN, 4096
    );

    for(int i=0; i < NUMOSCS; i++) {
        oscs[i] = LPPulsarOsc.create(
            NUMWTS, ringbuf->data, ringbuf->length, wt_onsets, wt_lengths, 
            NUMWINS, win->data, win->length, win_onsets, win_lengths
        );

        oscs[i]->samplerate = SR;
        oscs[i]->freq = 42.f * (i+1);
        oscs[i]->saturation = 0.95;
        //oscs[i]->pulsewidth = LPRand.rand(0.01f, 1);
        oscs[i]->wavetable_morph_freq = LPRand.rand(0.01f, 1.f);
        oscs[i]->burst = NULL;

        //patch.display.SetCursor(0, i*20+40);
        //str = std::to_string(oscs[i]->freq);
        //patch.display.WriteString(cstr, Font_6x8, true);

    }

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    //patch.seed.StartLog();

    //patch.display.Update();

    while(1) {
        patch.DelayMs(1);
        process_encoder();
    }

    for(int o=0; o < NUMOSCS; o++) {
        LPPulsarOsc.destroy(oscs[o]);
    }
}

