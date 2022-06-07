#include "daisy_pod.h"

extern "C" {
#include "../../../libpippi/src/pippicore.h"
#include "../../../libpippi/src/oscs.phasor.h"
#include "../../../libpippi/src/oscs.table.h"
}

#define POOLSIZE 67108864
#define BS 64

using namespace daisy;
using namespace daisy::seed;

DaisyPod   hw;
int SR;

lpphasorosc_t * osc1;
lpphasorosc_t * osc2;
lptableosc_t * env;
lpbuffer_t * win;
lpbuffer_t * lfo;
lpfloat_t lpfy = 0.f;

// Hook me up to the logic input 
// of a solenoid circuit... or something else.
GPIO sole;

unsigned char DSY_SDRAM_BSS pool[POOLSIZE];

// This runs at a fixed rate to prepare audio samples
void callback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size) {
    lpfloat_t sample, amp;

    hw.ProcessAllControls();

    amp = LPTableOsc.process(env);
    //freq = LPTableOsc.process(lfo);

    if(env->gate == 1) sole.Toggle();
    //sole.Write(env->gate == 1);

    for(size_t i = 0; i < size; i += 2) {
        sample = LPPhasorOsc.process(osc1) * 0.2;
        sample += LPPhasorOsc.process(osc2) * 0.2;
        sample = lpzapgremlins(LPFX.lpf1(sample * 0.5f, &lpfy, 80.f, SR));
        sample *= amp;

        out[i]   = sample;
        out[i+1] = sample;
    }
}

int main(void) {
    hw.Init();
    hw.SetAudioBlockSize(BS);
    SR = (int)hw.AudioSampleRate();

    sole.Init(D2, GPIO::Mode::OUTPUT);

    LPMemoryPool.init((unsigned char *)pool, POOLSIZE);

    osc1 = LPPhasorOsc.create();
    osc1->samplerate = SR;
    osc1->freq = 657.f;

    osc2 = LPPhasorOsc.create();
    osc2->samplerate = SR;
    osc2->freq = 555.f;

    lfo = LPWindow.create(WIN_SINE, 512);
    win = LPWindow.create(WIN_RSAW, 512);
    env = LPTableOsc.create(win);
    env->samplerate = SR;
    env->freq = 300.f;

    hw.StartAdc();
    hw.StartAudio(callback);

    while(1) {}
}
