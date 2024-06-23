#include "astrid.h"

#define NAME "simplesea"

#define SR 48000
#define CHANNELS 2
#define ADC_LENGTH 30

int trigger_callback(void * arg) {
    lpmsg_t msg = {0};
    double now = 0;
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    syslog(LOG_ERR, "INSTRUMENT TRIGGER CALLBACK %s\n", instrument->name);

    strncpy(msg.instrument_name, "simplesea", LPMAXNAME-1);

    if(lpscheduler_get_now_seconds(&now) < 0) return -1;
    msg.initiated = now;

    msg.type = LPMSG_PLAY;
    send_message(instrument->qname, msg);

    msg.type = LPMSG_TRIGGER;
    msg.scheduled = 0.02f;
    msg.flags = LPFLAG_IS_SCHEDULED;
    send_message(instrument->qname, msg);

    return 0;
}


int renderer_callback(void * arg) {
    lpbuffer_t * out;
    lpinstrument_t * instrument = (lpinstrument_t *)arg;

    size_t l = SR * 0.1;

    out = LPBuffer.create(l, instrument->channels, SR);
    syslog(LOG_INFO, "INSTRUMENT CALLBACK reading from ringbuffer\n");
    if(lpsampler_read_ringbuffer_block(instrument->adcname, instrument->adcbuf, l*2, out) < 0) {
        return -1;
    }

    syslog(LOG_ERR, "INSTRUMENT CALLBACK filled buffer with value 10 %f\n", out->data[10]);
    LPFX.norm(out, 0.01);

    if(send_render_to_mixer(instrument, out) < 0) {
        return -1;
    }

    LPBuffer.destroy(out);

    return 0;
}

int main() {
    lpinstrument_t * instrument;
    
    // Set the callbacks for streaming, async renders and param updates
    if((instrument = astrid_instrument_start(NAME, CHANNELS, 0, 
                    ADC_LENGTH, ADC_LENGTH, 
                    NULL, NULL, NULL, NULL, 
                    renderer_callback, NULL, trigger_callback)) == NULL) {
        fprintf(stderr, "Could not start instrument: (%d) %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* twiddle thumbs until shutdown */
    while(instrument->is_running) {
        astrid_instrument_tick(instrument);
    }

    /* stop jack and cleanup threads */
    if(astrid_instrument_stop(instrument) < 0) {
        fprintf(stderr, "There was a problem stopping the instrument. (%d) %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Done!\n");
    return 0;
}
