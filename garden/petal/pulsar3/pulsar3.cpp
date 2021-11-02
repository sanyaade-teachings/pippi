#include <stdio.h>
#include <string.h>
#include "daisy_petal.h"
#include "daisysp.h"

extern "C" {
#include "../../libpippi/src/pippicore.h"
#include "../../libpippi/src/oscs.sine.h"
#include "../../libpippi/src/oscs.pulsar.h"
}

#define POOLSIZE 67108864
#define BS 64
#define VOICES 2

using namespace daisy;
using namespace daisysp;


void UpdateLeds();

// Declare a local daisy_petal for hardware access
DaisyPetal hw;

// Variable for tracking encoder increments since there is it's not a continuous control like the rest.
int SR;

lppulsarosc_t * oscs[VOICES];
lpsineosc_t * lfos[VOICES];

char WTSTACK[] = "sine,square,tri,sine"; /* 21 */
char WINSTACK[] = "sine,hann,sine"; /* 15 */
char BURST[] = "1,1,0,1"; /* 8 */

unsigned char DSY_SDRAM_BSS pool[POOLSIZE];

// This runs at a fixed rate to prepare audio samples
void callback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size)
{
    //int32_t inc;
    lpfloat_t lfov;
    lpfloat_t sample = 0.f;
    hw.ProcessDigitalControls();
    hw.ProcessAnalogControls();

    // Handle Enc
    //inc = hw.encoder.Increment();

    // Audio Rate Loop
    for(size_t i = 0; i < size; i += 2) {
        sample = 0.f;
        for(int v=0; v < VOICES; v++) {
            //lfov = lpsvf(LPSineOsc.process(lfos[v]), 0, 1, -1, 1);
            //oscs[v]->wts->pos = lfov;
            //oscs[v]->wins->pos = lfov;
            sample += LPPulsarOsc.process(oscs[v]) * (1.f / VOICES) * 0.99f;
        }

        out[i]   = sample;
        out[i+1] = sample;
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

    for(int i=0; i < VOICES; i++) {
        lfos[i] = LPSineOsc.create();
        lfos[i]->samplerate = SR;
        lfos[i]->freq = LPRand.rand(0.0001, 0.1);

        oscs[i] = LPPulsarOsc.create();
        oscs[i]->samplerate = SR;
        oscs[i]->freq = 200.f;
        oscs[i]->saturation = 1;
        oscs[i]->pulsewidth = LPRand.rand(0.01, 1);
        oscs[i]->wts = LPWavetable.create_stack(WTSTACK, 21, 4096);
        oscs[i]->wins = LPWindow.create_stack(WINSTACK, 15, 4096);
        oscs[i]->burst = LPArray.create_from(BURST, 8);
    }

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

    for(int v=0; v < VOICES; v++) {
        LPPulsarOsc.destroy(oscs[v]);
    }
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
