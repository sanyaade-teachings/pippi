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
    out = LPBuffer.create(src->length*2, src->channels, src->samplerate);

    for(c=0; c < src->channels; c++) {
        cross[c] = LPCrossingFollower.create();
        wavesets[c] = LPBuffer.create(MAX_WAVESET, 1, src->samplerate);
        wavesets[c]->boundry = 0;
    }

    for(i=0; i < src->length; i++) {
        for(c=0; c < src->channels; c++) {
            LPCrossingFollower.process(cross[c], src->data[i * src->channels + c]);

            if(cross[c]->ws_transition && (i % 2 == 0)) {
                /* copy waveset to output buffer */
                for(j=0; j < wavesets[c]->pos; j++) {
                    out->data[(wavesets[c]->boundry+j) * src->channels + c] = src->data[((i-wavesets[c]->pos) + j) * src->channels + c];
                }
                /* increment the read index and add a fixed padding */
                wavesets[c]->boundry += wavesets[c]->pos + 222;
                if(wavesets[c]->boundry >= wavesets[c]->length) {
                    wavesets[c]->boundry -= wavesets[c]->length;
                }

                /* reset the position - new waveset */
                wavesets[c]->pos = 0;
            } else {
                wavesets[c]->pos += 1;
            }
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
