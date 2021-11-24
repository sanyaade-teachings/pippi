#include "daisysp.h"
#include "daisy_petal.h"

extern "C" {
#include "../../../libpippi/src/pippicore.h"
#include "../../../libpippi/src/oscs.tape.h"
#include "../../../libpippi/src/microsound.h"
}


#define BS 1024
#define CHANNELS 2

#define POOLSIZE 67108864

#define VOICES 1

#define NUMGRAINS 1
#define RBSIZE 100000
#define MAXGRAINLENGTH 10000
#define MINGRAINLENGTH 4800


#define NUM_KNOBS 6
#define NUM_BUTTONS 7

using namespace daisysp;
using namespace daisy;

void UpdateLeds();

// Knobs & switches & encoders etc
typedef struct controls_t {
    float knob[NUM_KNOBS];
    float button[NUM_BUTTONS];
    float enc;
    float exp;
} controls_t;

unsigned char DSY_SDRAM_BSS pool[POOLSIZE];

int SR;
uint32_t now, last_led_update;
uint32_t led_period = 5;

lpformation_t * formation;

controls_t controls;
/*grain_t * grains[NUM_GRAINS * 2];*/

DaisyPetal hw;


void callback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size) {

    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    size_t frames = size/CHANNELS;

    LPRingBuffer.writefrom(formation->rb, (lpfloat_t *)in, frames, (int)CHANNELS);

    formation->speed = (lpfloat_t)(hw.GetKnobValue(hw.KNOB_1) * 1.99 + 0.01);
    for(size_t i=0; i < frames; i++) {
        LPFormation.process(formation);
        out[i * CHANNELS + 0] = formation->current_frame->data[0];
        out[i * CHANNELS + 1] = formation->current_frame->data[1];
    }
}


int main(void) {
    hw.Init();
    hw.SetAudioBlockSize(BS);
    SR = (int)hw.AudioSampleRate();

    LPRand.seed(888);
    LPMemoryPool.init((unsigned char *)pool, POOLSIZE);

    formation = LPFormation.create(NUMGRAINS, MAXGRAINLENGTH, MINGRAINLENGTH, RBSIZE, CHANNELS, SR);

    hw.StartAdc();
    hw.StartAudio(callback);

    for(;;) {
        /*
        midi.Listen();
        while(midi.HasEvents()) {
            midimessage(midi.PopEvent());
        }
        */
        now = System::GetNow();

        // Update LEDs (Vegas Mode)
        if(now - last_led_update > led_period) {
            last_led_update = now;
            UpdateLeds();
        }

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
