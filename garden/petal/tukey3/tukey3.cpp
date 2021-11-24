#include <stdio.h>
#include <string.h>
#include "daisy_petal.h"
#include "daisysp.h"

extern "C" {
#include "../../../libpippi/src/pippicore.h"
#include "../../../libpippi/src/oscs.sine.h"
#include "../../../libpippi/src/oscs.tukey.h"
}

#define POOLSIZE 67108864
#define BS 64

#define KNOB_ID "KNOB_"
#define SW_ID "SWITCH_"
#define ENC_ID "ENCODER_"

using namespace daisy;
using namespace daisysp;


void UpdateLeds();

// Declare a local daisy_petal for hardware access
DaisyPetal hw;

// Variable for tracking encoder increments since there is it's not a continuous control like the rest.
int32_t enc_tracker;
int SR;

lptukeyosc_t * osc1;
lptukeyosc_t * osc2;
lptukeyosc_t * osc3;

lpsineosc_t * lfo1;
lpsineosc_t * lfo2;
lpsineosc_t * lfo3;

lpfloat_t f1 = 73.3333333f;
lpfloat_t f2 = 88.f * 2;
lpfloat_t f3 = 110.f * 2;

float k1, k2, k3, k4, k5, k6;
bool footswitchOn;

unsigned char DSY_SDRAM_BSS pool[POOLSIZE];

// This runs at a fixed rate to prepare audio samples
void callback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size) {
    int32_t inc;
    lpfloat_t outL, outR;
    lpfloat_t l1, l2, l3;
    lpfloat_t o1, o2, o3;

    hw.ProcessAllControls();

    inc = hw.encoder.Increment();

    k1 = hw.knob[0].Process();
    k2 = hw.knob[1].Process();
    k3 = hw.knob[2].Process();
    k4 = hw.knob[3].Process();
    k5 = hw.knob[4].Process();
    k6 = hw.knob[5].Process();

    footswitchOn ^= hw.switches[0].RisingEdge();

    l1 = lpsvf(LPSineOsc.process(lfo1), 1, 20, -1, 1) * k3;
    l2 = lpsvf(LPSineOsc.process(lfo2), 1, 30, -1, 1) * k4;
    l3 = lpsvf(LPSineOsc.process(lfo3), 1, 10, -1, 1) * k5;

    osc1->shape = lpsvf(k1, 0, 1, -1, 1);
    osc2->shape = lpsvf(k2, 0, 1, -1, 1);
    osc3->shape = lpsvf(k6, 0, 1, -1, 1);

    osc1->freq = f1 + l1;
    osc2->freq = f2 + l2;
    osc3->freq = f3 + l3;

    // Audio Rate Loop
    for(size_t i = 0; i < size; i += 2) {
        o1 = LPTukeyOsc.process(osc1) * 0.2;
        o2 = LPTukeyOsc.process(osc2) * 0.2;
        o3 = LPTukeyOsc.process(osc3) * 0.2;

        outL = o1 + (o2 * 0.5);
        outR = o3 + (o2 * 0.5);

        out[i]   = outL;
        out[i+1] = outR;
    }
}

int main(void)
{
    uint32_t led_period, now;
    uint32_t last_led_update;

    // LED Freq = 60Hz
    led_period = 5;

    hw.Init();
    //hw.SetAudioBlockSize(BS);
    SR = (int)hw.AudioSampleRate();

    LPMemoryPool.init((unsigned char *)pool, POOLSIZE);

    osc1 = LPTukeyOsc.create();
    osc2 = LPTukeyOsc.create();
    osc3 = LPTukeyOsc.create();

    lfo1 = LPSineOsc.create();
    lfo2 = LPSineOsc.create();
    lfo3 = LPSineOsc.create();

    lfo1->samplerate = SR;
    lfo1->freq = 1.5f;
    lfo2->samplerate = SR;
    lfo2->freq = 2.f;
    lfo3->samplerate = SR;
    lfo3->freq = 3.f;

    osc1->samplerate = SR;
    osc1->freq = 200.f;
    osc1->shape = 0.44;

    osc2->samplerate = SR;
    osc2->freq = 300.f;
    osc2->shape = 0.7;

    osc3->samplerate = SR;
    osc3->freq = 400.f;
    osc3->shape = 0.03;

    last_led_update = now = System::GetNow();

    hw.StartAdc();
    hw.StartAudio(callback);

    while(1) {
        now = System::GetNow();

        // Update LEDs (Vegas Mode)
        if(now - last_led_update > led_period) {
            last_led_update = now;
            UpdateLeds();
        }
    }

    LPTukeyOsc.destroy(osc1);
    LPTukeyOsc.destroy(osc2);
    LPTukeyOsc.destroy(osc3);
}

void UpdateLeds()
{
    uint32_t now;
    now = System::GetNow();
    hw.ClearLeds();
    // Use now as a source for time so we don't have to use any global vars
    // First gradually pulse all 4 Footswitch LEDs
    for(size_t i = 0; i < hw.FOOTSWITCH_LED_LAST; i++)
    {
        size_t total, base;
        total        = 511;
        base         = total / hw.FOOTSWITCH_LED_LAST;
        float bright = (float)((now + (i * base)) & total) / (float)total;
        hw.SetFootswitchLed(static_cast<DaisyPetal::FootswitchLed>(i), bright);
    }
    // And now the ring
    for(size_t i = 0; i < hw.RING_LED_LAST; i++)
    {
        float    rb, gb, bb;
        uint32_t total, base;
        uint32_t col;
        col = (now >> 10) % 6;
        //        total = 8191;
        //        base  = total / (hw.RING_LED_LAST);
        total        = 1023;
        base         = total / hw.RING_LED_LAST;
        float bright = (float)((now + (i * base)) & total) / (float)total;
        bright       = 1.0f - bright;
        switch(col)
        {
            case 0:
                rb = bright;
                gb = 0.0f;
                bb = 0.0f;
                break;
            case 1:
                rb = 0.6f * bright;
                gb = 0.0f;
                bb = 0.7f * bright;
                break;
            case 2:
                rb = 0.0f;
                gb = bright;
                bb = 0.0f;
                break;
            case 3:
                rb = 0.0f;
                gb = 0.0f;
                bb = bright;
                break;
            case 4:
                rb = 0.75f * bright;
                gb = 0.75f * bright;
                bb = 0.0f;
                break;
            case 5:
                rb = 0.0f;
                bb = 0.85f * bright;
                gb = 0.85f * bright;
                break;

            default: rb = gb = bb = bright; break;
        }

        hw.SetRingLed(static_cast<DaisyPetal::RingLed>(i), rb, gb, bb);
    }
    hw.UpdateLeds();
}
