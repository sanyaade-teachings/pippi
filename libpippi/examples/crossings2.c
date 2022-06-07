#include "pippi.h"

#define MAX_CHANNELS 2
#define MAX_WAVESET 4096
#define STACK_SIZE 10

int main() {
    size_t i;
    int c, j;
    /*lpfloat_t pos;*/
    lpbuffer_t * src;
    lpbuffer_t * out;
    lpbuffer_t * waveset;
    /*lpbuffer_t * freq;*/
    lppulsarosc_t * oscs[MAX_CHANNELS];
    lpstack_t * wavesets[MAX_CHANNELS];
    lpcrossingfollower_t * cross[MAX_CHANNELS];
    int stack_read_index[MAX_CHANNELS];
    int waveset_write_pos[MAX_CHANNELS];
    int src_offset[MAX_CHANNELS];

    /*src = LPSoundFile.read("../../pippi/tests/sounds/living.wav");*/
    src = LPSoundFile.read("../../pippi/tests/sounds/sine100hz30s.wav");
    out = LPBuffer.create(src->length, src->channels, src->samplerate);
    waveset = LPBuffer.create(MAX_WAVESET, 1, src->samplerate);
    /*freq = LPWindow.create(WIN_SINE, 4096);*/

    for(c=0; c < src->channels; c++) {
        cross[c] = LPCrossingFollower.create();
        wavesets[c] = LPBuffer.create_stack(STACK_SIZE, MAX_WAVESET, 1, src->samplerate);
        stack_read_index[c] = 0;
        waveset_write_pos[c] = 0;

        oscs[c] = LPPulsarOsc.create();
        oscs[c]->samplerate = src->samplerate;
        oscs[c]->freq = 100.f;
        oscs[c]->saturation = 1.f;
        oscs[c]->pulsewidth = 1.f;
        oscs[c]->wins = LPWindow.create_stack(2, WIN_SINE, WIN_HANN);
        oscs[c]->wts = wavesets[c];
        src_offset[c] = 0;
    }

    for(i=0; i < src->length; i++) {
        /*pos = (lpfloat_t)i / src->length;*/
        for(c=0; c < src->channels; c++) {
            LPCrossingFollower.process(cross[c], src->data[i * src->channels + c]);

            if(cross[c]->ws_transition) {
                /* copy waveset to stack */
                waveset = wavesets[c]->stack[stack_read_index[c]];
                for(j=0; j < waveset_write_pos[c]; j++) {
                    waveset->data[j] = src->data[(src_offset[c] + j) * src->channels + c];
                }
                /* reset the position - new waveset */
                src_offset[c] += waveset_write_pos[c];
                waveset_write_pos[c] = 0;
                stack_read_index[c] = (stack_read_index[c] + 1) % wavesets[c]->length;
            } else {
                waveset_write_pos[c] += 1;
            }

            if(waveset_write_pos[c] > MAX_WAVESET) waveset_write_pos[c] = 0;

            /* Write to output with pulsarosc fed by waveset stack */
            /*oscs[c]->freq = LPInterpolation.linear_pos(freq, pos) * 100 + 80;*/
            out->data[i * src->channels + c] = LPPulsarOsc.process(oscs[c]) * 0.15f;
        }
    }

    LPSoundFile.write("renders/crossings2-out.wav", out);

    for(c=0; c < src->channels; c++) {
        LPCrossingFollower.destroy(cross[c]);
        /*LPBuffer.destroy_stack(wavesets[c]);*/
        LPPulsarOsc.destroy(oscs[c]);
    }

    LPBuffer.destroy(out);
    LPBuffer.destroy(src);

    return 0;
}
