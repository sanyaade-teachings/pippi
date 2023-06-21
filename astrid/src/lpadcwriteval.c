#include "astrid.h"

int main(int argc, char * argv[]) {
    lpipc_buffer_t * buf;
    int position, channel, adc_shmid;
    size_t insert_pos;
    lpfloat_t sample;

    if(argc != 5) {
        printf("Usage: %s <adc_shmid:int> <pos:int> <channel:int> <value:float> (%d)\n", argv[0], argc);
    }

    adc_shmid = atoi(argv[1]);
    position = atoi(argv[2]);
    channel = atoi(argv[3]);
    sample = (lpfloat_t)atof(argv[4]);
    insert_pos = (size_t)(position * channel);

    printf("Writing %f to ADC buffer at position %d channel %d...\n", sample, position, channel);

    if(lpipc_buffer_aquire(LPADC_BUFFER_PATH, &buf, adc_shmid) < 0) {
        fprintf(stderr, "Could not aquire ADC buffer\n");
        return 1;
    }

    memcpy(buf->data + (sizeof(lpfloat_t) * insert_pos), (char*)(&sample), sizeof(lpfloat_t));

    if(lpipc_buffer_release(LPADC_BUFFER_PATH, (void *)buf) < 0) {
        fprintf(stderr, "Could not release ADC buffer\n");
        return 1;
    }

    printf("Wrote value to ADC buffer\n");

    return 0;
}
