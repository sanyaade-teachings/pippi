#include "pippi.h"

#define MAX_CHANNELS 2
#define MAX_WAVESET 4096

int main() {
    size_t i, j;
    int c;
    lpbuffer_t * src;
    lpbuffer_t * out;
    lpbuffer_t * wavesets[MAX_CHANNELS];
    lpcrossingfollower_t * cross[MAX_CHANNELS];

    src = LPSoundFile.read("../../pippi/tests/sounds/living.wav");
    out = LPBuffer.create(src->length, src->channels, src->samplerate);

    for(c=0; c < src->channels; c++) {
        cross[c] = LPCrossingFollower.create();
        wavesets[c] = LPBuffer.create(MAX_WAVESET, 1, src->samplerate);
    }

    for(i=0; i < src->length; i++) {
        for(c=0; c < src->channels; c++) {
            LPCrossingFollower.process(cross[c], src->data[i * src->channels + c]);

            if(cross[c]->ws_transition) {
                /* copy waveset to output buffer */
                for(j=0; j < wavesets[c]->pos; j++) {
                    out->data[(wavesets[c]->boundry+j) * src->channels + c] = src->data[i * src->channels + c];
                }
                /* reset the position - new waveset */
                wavesets[c]->pos = 0;
            } 

            wavesets[c]->pos += 1;
            wavesets[c]->boundry += 1;
        }
    }

    LPSoundFile.write("renders/crossings-out.wav", out);

    for(c=0; c < src->channels; c++) {
        LPCrossingFollower.destroy(cross[c]);
        LPBuffer.destroy(wavesets[c]);
    }

    LPBuffer.destroy(out);
    LPBuffer.destroy(src);

    return 0;
}
