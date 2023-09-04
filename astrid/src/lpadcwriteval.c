#include "astrid.h"

int main(int argc, char * argv[]) {
    lpipc_buffer_t * buf;
    int position, channel;
    size_t insert_pos;
    lpfloat_t sample;

    if(argc != 4) {
        printf("Usage: %s <pos:int> <channel:int> <value:float> (%d)\n", argv[0], argc);
    }

    position = atoi(argv[1]);
    channel = atoi(argv[2]);
    sample = (lpfloat_t)atof(argv[3]);
    insert_pos = (size_t)(position * channel);

    printf("Writing %f to ADC buffer at position %d channel %d...\n", sample, position, channel);

    if(lpipc_buffer_aquire(LPADC_BUFFER_PATH, &buf) < 0) {
        fprintf(stderr, "Could not aquire ADC buffer\n");
        return 1;
    }

    memcpy(buf->data + (sizeof(lpfloat_t) * insert_pos), (char*)(&sample), sizeof(lpfloat_t));

    if(lpipc_buffer_release(LPADC_BUFFER_PATH) < 0) {
        fprintf(stderr, "Could not release ADC buffer\n");
        return 1;
    }

    printf("Wrote value to ADC buffer\n");

    return 0;
}
