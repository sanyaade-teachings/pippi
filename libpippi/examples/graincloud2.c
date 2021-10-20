#include "pippi.h"

#define SR 48000
#define CHANNELS 2

int main() {
    size_t i, c, length, numgrains, maxgrainlength, mingrainlength;
    lpbuffer_t * snd;
    lpbuffer_t * out;
    lpcloud_t * cloud;

    length = 60 * 5 * SR;
    numgrains = 1;
    maxgrainlength = SR;
    mingrainlength = SR/10.;

    out = LPBuffer.create(length, CHANNELS, SR);
    snd = LPSoundFile.read("examples/linus.wav");
    cloud = LPCloud.create(numgrains, maxgrainlength, mingrainlength, length, CHANNELS, SR);

    LPRingBuffer.write(cloud->rb, snd);

    /* Render each frame of the graincloud */
    for(i=0; i < length; i++) {
        LPCloud.process(cloud);
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = cloud->current_frame->data[c];
        }
    }

    LPSoundFile.write("renders/graincloud2-out.wav", out);

    LPBuffer.destroy(out);
    LPBuffer.destroy(snd);
    LPCloud.destroy(cloud);

    return 0;
}
