#include "pippi.h"

int main() {
    lpbuffer_t * out;
    lpbuffer_t * ding;
    lpscheduler_t * s;
    int channels = 2;

    ding = LPSoundFile.read("examples/linus.wav");
    s = LPScheduler.create(channels);


    while(1) {
        LPScheduler.schedule_event(s, ding, 0);
        LPScheduler.tick(s);

        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = s->current_frame[c];
        }
    }
}
