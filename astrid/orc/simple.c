#include "astrid.h"

#define NAME "simplesea"

#define SR 48000
#define CHANNELS 2
#define ADC_LENGTH 30

lpbuffer_t * renderer_callback(void * arg) {
    lpbuffer_t * out;
    lpinstrument_t * instrument = (lpinstrument_t *)arg;

    out = LPBuffer.create(SR, instrument->channels, SR);
    if(lpsampler_read_ringbuffer_block("simplesea-adc", 0, out->length, instrument->channels, out->data) < 0) {
        return NULL;
    }

    return out;
}

int main() {
    lpinstrument_t * instrument;
    
    // Set the callbacks for streaming, async renders and param updates
    if((instrument = astrid_instrument_start(NAME, CHANNELS, ADC_LENGTH, NULL, NULL, renderer_callback, NULL)) == NULL) {
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
