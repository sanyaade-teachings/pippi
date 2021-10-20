#include "daisy_field.h"

extern "C" {
#include "../../libpippi/src/pippicore.h"
#include "../../libpippi/src/oscs.tape.h"
#include "../../libpippi/src/microsound.h"
}

/* Set the memory pool size to 64MB */
#define POOLSIZE 67108864
#define CHANNELS 2
#define BS 1
#define NUMGRAINS 3
#define RBSIZE 480000
#define MAXGRAINLENGTH 48000
#define MINGRAINLENGTH 48

using namespace daisy;

void UpdateLeds();

//MidiHandler midi;
DaisyField hw;
lpcloud_t * cloud;
lptapeosc_t * osc;
lpbuffer_t * rb;
int SR;

unsigned char DSY_SDRAM_BSS pool[POOLSIZE];


void callback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size) {

    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    size_t frames = size/CHANNELS;

    LPRingBuffer.writefrom(cloud->rb, (lpfloat_t *)in, frames, (int)CHANNELS);

    cloud->speed = (lpfloat_t)(hw.GetKnobValue(hw.KNOB_1) * 1.99 + 0.01);
    for(size_t i=0; i < frames; i++) {
        LPCloud.process(cloud);
        out[i * CHANNELS + 0] = cloud->current_frame->data[0];
        out[i * CHANNELS + 1] = cloud->current_frame->data[1];
    }
}

/*
void midimessage(MidiEvent m) {
    if(m.type == NoteOn) {
        NoteOnEvent p = m.AsNoteOn();
        cloud->maxlength = (size_t)((p.note / 128.f) * SR + 1);
        cloud->minlength = (size_t)(cloud->maxlength * 0.1 + 1);
    }
}
*/


int main(void) {
    hw.Init();
    hw.SetAudioBlockSize(BS);
    SR = (int)hw.AudioSampleRate();
    //midi.Init(MidiHandler::INPUT_MODE_UART1, MidiHandler::OUTPUT_MODE_NONE);

    //LPRand.rand_base = LPRand.logistic_rand;
    //LPRand.logistic_seed = 3.998f;
    LPRand.seed(888);

    LPMemoryPool.init((unsigned char *)pool, POOLSIZE);

    cloud = LPCloud.create(NUMGRAINS, MAXGRAINLENGTH, MINGRAINLENGTH, RBSIZE, CHANNELS, SR);
    //rb = LPRingBuffer.create(SR*3, CHANNELS, SR);
    //osc = LPTapeOsc.create(rb, 4800.f);
    //cloud->osc->offset = -4800.f;

    //midi.StartReceive();
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
