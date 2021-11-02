#include "daisy_field.h"
#include "daisysp.h"

extern "C" {
#include "../../libpippi/src/pippicore.h"
#include "../../libpippi/src/ringbuffer.h"
}

/* Set the memory pool size to 64MB */
#define POOLSIZE 67108864
#define CHANNELS 2
#define BS 128
#define NUMGRAINS 40
#define MAXGRAINLENGTH 48000
#define MINGRAINLENGTH 4800

using namespace daisy;
using namespace daisysp;

typedef struct grain_t {
    size_t elapsed;
    size_t length;
    size_t offset;
    lpfloat_t panleft;
    lpfloat_t panright;
    int playing;
} grain_t;

DaisyField hw;
ringbuffer_t * rb;
grain_t * grains[NUMGRAINS];
buffer_t * hann;
int SR;
lpfloat_t grainamp = (1.f / NUMGRAINS) * 1.5;

unsigned char DSY_SDRAM_BSS pool[POOLSIZE];


void AudioCallback(float * in, float * out, size_t size) {
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();
    size_t frames = size/2;
    size_t readpos = 0;
    lpfloat_t amp = 1.f;
    lpfloat_t pos = 0.f;
    lpfloat_t pan = 0.5f;
    lpfloat_t leftout = 0.f;
    lpfloat_t rightout = 0.f;

    int numplaying = 0;
    int tostart = NUMGRAINS;

    LPRingBuffer.writefrom(rb, (lpfloat_t *)in, frames, (int)CHANNELS);

    for(int g=0; g < NUMGRAINS; g++) {
        if(grains[g]->playing == 1) numplaying += 1;
    }

    if(numplaying < tostart) {
        tostart = tostart - numplaying;
        for(int g=0; g < NUMGRAINS; g++) {
            if(grains[g]->playing == 0 && tostart > 0) {
                grains[g]->playing = 1;            
                grains[g]->elapsed = (size_t)0;            
                grains[g]->length = (size_t)Rand.randint(MINGRAINLENGTH, MAXGRAINLENGTH);
                grains[g]->offset = (size_t)Rand.randint(0, rb->buf->length - grains[g]->length);
                pan = Rand.rand(0.f, 1.f);
                grains[g]->panleft = sqrt(pan);
                grains[g]->panright = sqrt(1.f - pan);
                tostart -= 1;
            }
        }
    }

    for(size_t i=0; i < frames; i++) {
        leftout = 0.f;
        rightout = 0.f;

        for(int g=0; g < NUMGRAINS; g++) {
            if(grains[g]->playing == 1) {
                readpos = (rb->pos - (grains[g]->length + grains[g]->offset)) + i;
                readpos = readpos % rb->buf->length;

                pos = (lpfloat_t)grains[g]->elapsed / grains[g]->length;
                amp = Interpolation.linear_pos(hann, pos) * grainamp;

                leftout += rb->buf->data[readpos * CHANNELS + 0] * amp * grains[g]->panleft;
                rightout += rb->buf->data[readpos * CHANNELS + 1] * amp * grains[g]->panright;

                grains[g]->elapsed += 1;

                if(grains[g]->elapsed >= grains[g]->length) {
                    grains[g]->playing = 0;
                }
            }
        }

        out[i * CHANNELS + 0] = leftout;
        out[i * CHANNELS + 1] = rightout;
    }
}

int main(void) {
    hw.Init();
    hw.SetAudioBlockSize(BS);
    SR = (int)hw.AudioSampleRate();

    Rand.rand_base = Rand.logistic_rand;
    Rand.logistic_seed = 3.998f;

    MemoryPool.init((unsigned char *)pool, POOLSIZE);

    hann = Wavetable.create("hann", 4096);
    rb = LPRingBuffer.create(SR*4, CHANNELS, SR);

    for(int g=0; g < NUMGRAINS; g++) {
        grains[g] = (grain_t *)MemoryPool.alloc(1, sizeof(grain_t));
        grains[g]->length = MAXGRAINLENGTH;
        grains[g]->offset = 0;
        grains[g]->elapsed = 0;
        grains[g]->playing = 0;
        grains[g]->panleft = 0.5;
        grains[g]->panright = 0.5;
    }

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    /*
    for(;;) {
        //UpdateLeds(kvals);
        System::Delay(1);
        //hw.seed.dac.WriteValue(DacHandle::Channel::ONE, hw.GetKnobValue(0) * 4095);
        //hw.seed.dac.WriteValue(DacHandle::Channel::TWO, hw.GetKnobValue(1) * 4095);
    }
    */
}
