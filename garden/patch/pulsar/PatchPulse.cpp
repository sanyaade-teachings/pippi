#include "daisy_patch.h"
#include "daisysp.h"
#include <string>

#define LP_FLOAT
#include <pulsar.h>

using namespace daisy;
using namespace daisysp;

#define NUM_VOICES 4

// Hardware
DaisyPatch hw;

Pulsar pulsars[NUM_VOICES];

// Pulsar setup
int tablesize = 4096;
int samplerate = 44100;

lpfloat_t modfreq = 0.03f;
lpfloat_t morphfreq = 0.3f;
lpfloat_t freq1 = 220.0f;
lpfloat_t freq2 = 330.0f;
//lpfloat_t freq3 = 440.0f;
//lpfloat_t freq4 = 550.0f;

char wts[] = "sine,square,tri,sine";
char wins[] = "sine,hann,sine";
char burst[] = "1,1,0,1";

Pulsar* p1 = init_pulsar(tablesize, freq1, modfreq, morphfreq, wts, wins, burst, samplerate);
Pulsar* p2 = init_pulsar(tablesize, freq2, modfreq, morphfreq + 0.1, wts, wins, burst, samplerate);
//Pulsar* p3 = init_pulsar(tablesize, freq3, modfreq, morphfreq + 0.05, wts, wins, burst, samplerate);
//Pulsar* p4 = init_pulsar(tablesize, freq4, modfreq, morphfreq + 0.2, wts, wins, burst, samplerate);


void AudioCallback(float **in, float **out, size_t blocksize) {
    float freq, trig;
    //float samplerate;

    // Assign Output Buffers
    float *out_left, *out_right;
    out_left  = out[0];
    out_right = out[1];

    //samplerate = hw.AudioSampleRate();
    hw.ProcessDigitalControls();
    hw.ProcessAnalogControls();


    // Handle Triggering
    trig = 0.0f;
    if(hw.encoder.RisingEdge() || hw.gate_input[DaisyPatch::GATE_IN_1].Trig())
        trig = 1.0f;

    // cc2 = hw.GetKnobValue(DaisyPatch::CTRL_2);
    // cc3 = hw.GetKnobValue(DaisyPatch::CTRL_3);
    // cc4 = hw.GetKnobValue(DaisyPatch::CTRL_4);

    for(size_t i = 0; i < blocksize; i++) {
        freq = 40.0f + hw.GetKnobValue(DaisyPatch::CTRL_1) * 600.0f;
        p1->freq = (lpfloat_t)(freq);
        p2->freq = (lpfloat_t)(freq+110.0f);
        //p3->freq = (lpfloat_t)(freq+220.0f);
        //p4->freq = (lpfloat_t)(freq+330.0f);

        //out_left[i]  = (float)(process_pulsar_sample(p1) + process_pulsar_sample(p2)) * 0.5f;
        //out_right[i]  = (float)(process_pulsar_sample(p3) + process_pulsar_sample(p4)) * 0.5f;
        out_left[i]  = (float)process_pulsar_sample(p1);
        out_right[i]  = (float)process_pulsar_sample(p2);
    }
}

int main(void)
{
    // Init everything.
    //float samplerate;
    hw.Init();
    //samplerate = hw.AudioSampleRate();

    //briefly display the module name
    std::string str  = "Patch Pulse";
    char *      cstr = &str[0];
    hw.display.WriteString(cstr, Font_7x10, true);
    hw.display.Update();
    hw.DelayMs(1000);

    // Start the ADC and Audio Peripherals on the Hardware
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    for(;;)
    {
        hw.DisplayControls(false);

    }
}
