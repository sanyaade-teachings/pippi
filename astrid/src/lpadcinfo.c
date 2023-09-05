#include "astrid.h"

int main(__attribute__((unused)) int argc, __attribute__((unused)) char * argv[]) {
    lpipc_buffer_t * buf;

    printf("ADC buffer:\n");

    if(lpipc_buffer_aquire(LPADC_BUFFER_PATH, &buf) < 0) {
        fprintf(stderr, "Could not aquire ADC buffer\n");
        return 1;
    }

    printf("    position:\t%ld\n", buf->pos);
    printf("    left chan:\t%f\n", buf->data[buf->pos]);
    printf("    right chan:\t%f\n", buf->data[buf->pos+1]);

    if(lpipc_buffer_release(LPADC_BUFFER_PATH) < 0) {
        fprintf(stderr, "Could not release ADC buffer\n");
        return 1;
    }

    return 0;
}
