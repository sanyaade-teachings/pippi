#include "daisy_petal.h"

extern "C" {
#include "../../../libpippi/src/pippicore.h"
#include "../../../libpippi/src/oscs.phasor.h"
}

#define POOLSIZE 67108864
#define BS 64

using namespace daisy;

DaisyPetal hw;
int SR;

lpphasorosc_t * osc1;
lpphasorosc_t * osc2;
lpfloat_t lpfy = 0.f;

unsigned char DSY_SDRAM_BSS pool[POOLSIZE];

void callback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size) {
    lpfloat_t outL, outR, sample;
    hw.ProcessAllControls();

    for(size_t i = 0; i < size; i += 2) {
        sample = LPPhasorOsc.process(osc1) * 0.2;
        sample += LPPhasorOsc.process(osc2) * 0.2;

        sample = LPFX.lpf1(sample * 0.5f, &lpfy, 80.f, SR);

        outL = sample;
        outR = sample;

        out[i]   = outL;
        out[i+1] = outR;
    }
}

int main(void) {
    hw.Init();
    SR = (int)hw.AudioSampleRate();

    LPMemoryPool.init((unsigned char *)pool, POOLSIZE);

    osc1 = LPPhasorOsc.create();
    osc1->samplerate = SR;
    osc1->freq = 54.f;

    osc2 = LPPhasorOsc.create();
    osc2->samplerate = SR;
    osc2->freq = 55.f;

    hw.StartAdc();
    hw.StartAudio(callback);

    while(1) {}

    LPPhasorOsc.destroy(osc1);
    LPPhasorOsc.destroy(osc2);
}

