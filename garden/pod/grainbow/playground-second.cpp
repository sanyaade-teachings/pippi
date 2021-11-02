#include "daisy_field.h"

extern "C" {
#include "../../libpippi/src/pippicore.h"
#include "../../libpippi/src/ringbuffer.h"
}

/* Set the memory pool size to 64MB */
#define POOLSIZE 67108864
#define CHANNELS 2
#define BS 128
#define NUMGRAINS 20
#define RBSIZE 262144
#define RBSIZEM 262143
#define MAXGRAINLENGTH 4800
#define MINGRAINLENGTH 480
#define WTLENGTH 48000
#define SPEED 0.5

using namespace daisy;

typedef struct grain_t {
    size_t length;
    size_t offset;
    size_t startpos;

    lpfloat_t phase;
    lpfloat_t phaseinc;
    lpfloat_t pan;
    lpfloat_t speed;

    lpfloat_t window_phase;
    lpfloat_t window_phaseinc;
} grain_t;

DaisyField hw;
ringbuffer_t * rb;
grain_t * grains[NUMGRAINS];
buffer_t * hann;
int SR;
lpfloat_t grainamp = (1.f / NUMGRAINS);

unsigned char DSY_SDRAM_BSS pool[POOLSIZE];


void AudioCallback(float * in, float * out, size_t size) {
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    size_t frames = size/CHANNELS;
    size_t readpos, idxa, idxb;

    lpfloat_t a, b, f, l, r;
    lpfloat_t amp = 1.f;
    lpfloat_t leftout = 0.f;
    lpfloat_t rightout = 0.f;

    LPRingBuffer.writefrom(rb, (lpfloat_t *)in, frames, (int)CHANNELS);

    for(size_t i=0; i < frames; i++) {
        leftout = 0.f;
        rightout = 0.f;

        for(int g=0; g < NUMGRAINS; g++) {
            if(grains[g]->phase >= grains[g]->length) {
                grains[g]->length = (size_t)Rand.randint(MINGRAINLENGTH, MAXGRAINLENGTH) * (1.f / SPEED);
                grains[g]->offset = (size_t)Rand.randint(0, rb->buf->length - grains[g]->length - 1);
                grains[g]->startpos = rb->pos - grains[g]->length + grains[g]->offset;

                grains[g]->window_phaseinc = (lpfloat_t)WTLENGTH / grains[g]->length;
                grains[g]->window_phase = 0.f;
                grains[g]->pan = Rand.rand(0.f, 1.f);

                grains[g]->phase = 0.f;
                grains[g]->phaseinc = SPEED;
            } else {
                readpos = grains[g]->startpos + grains[g]->phase;

                /* This little trick came from Hacker's Delight.
                 * For values where RBSIZE is a power of two, and 
                 * RBSIZEM is RBSIZE-1 it is the same as doing:
                 *
                 *      readpos = readpos % RBSIZE;
                 *
                 * (But without the division.)
                 */
                idxa = RBSIZE - (-readpos & RBSIZEM);
                idxb = RBSIZE - (-(readpos+1) & RBSIZEM);
                f = grains[g]->phase - (int)grains[g]->phase;

                a = rb->buf->data[idxa * CHANNELS + 0];
                b = rb->buf->data[idxa * CHANNELS + 2];
                l = (1.f - f) * a + (f * b);

                a = rb->buf->data[idxb * CHANNELS + 0];
                b = rb->buf->data[idxb * CHANNELS + 2];
                r = (1.f - f) * a + (f * b);

                amp = hann->data[(size_t)grains[g]->window_phase];

                leftout += l * amp * grains[g]->pan;
                rightout += r * amp * (1.f - grains[g]->pan);

                grains[g]->window_phase += grains[g]->window_phaseinc;
                if(grains[g]->window_phase >= WTLENGTH) {
                    grains[g]->window_phase = grains[g]->window_phase - WTLENGTH;
                }

                grains[g]->phase += grains[g]->phaseinc;
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

    //Rand.rand_base = Rand.logistic_rand;
    //Rand.logistic_seed = 3.998f;
    Rand.seed(888);

    MemoryPool.init((unsigned char *)pool, POOLSIZE);

    hann = Wavetable.create("hann", WTLENGTH);
    rb = LPRingBuffer.create(RBSIZE, CHANNELS, SR);

    for(int g=0; g < NUMGRAINS; g++) {
        grains[g] = (grain_t *)MemoryPool.alloc(1, sizeof(grain_t));
        grains[g]->length = MINGRAINLENGTH;
        grains[g]->offset = 0;
        grains[g]->phase = 0.f;
        grains[g]->phaseinc = SPEED;
        grains[g]->pan = 0.5;
        grains[g]->window_phase = 0;
        grains[g]->window_phaseinc = (lpfloat_t)WTLENGTH / MINGRAINLENGTH;
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
