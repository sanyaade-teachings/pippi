#include "daisy_pod.h"

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

//MidiHandler midi;
DaisyPod hw;
lpcloud_t * cloud;
lptapeosc_t * osc;
lpbuffer_t * rb;
int SR;

unsigned char DSY_SDRAM_BSS pool[POOLSIZE];

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{

    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    size_t frames = size/CHANNELS;

    LPRingBuffer.writefrom(cloud->rb, (lpfloat_t *)in, frames, (int)CHANNELS);

    //cloud->speed = (lpfloat_t)(hw.GetKnobValue(hw.KNOB_1) * 1.99 + 0.01);
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
    hw.StartAudio(AudioCallback);

    for(;;) {
        /*
        midi.Listen();
        while(midi.HasEvents()) {
            midimessage(midi.PopEvent());
        }
        */
    }
}
