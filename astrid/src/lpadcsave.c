#include "astrid.h"

int main(int argc, char * argv[]) {
    lpipc_buffer_t * buf;
    lpbuffer_t * out;
    char * out_path;
    int adc_shmid;

    if(argc != 3) {
        printf("Usage: %s <adc_shmid:int> <outpath.wav> (%d)\n", argv[0], argc);
    }

    printf("Saving ADC buffer...\n");

    adc_shmid = atoi(argv[1]);
    out_path = argv[2];

    if(lpipc_buffer_aquire(LPADC_BUFFER_PATH, &buf, adc_shmid) < 0) {
        fprintf(stderr, "Could not aquire ADC buffer\n");
        return 1;
    }

    lpipc_buffer_tolpbuffer(buf, &out);
    LPSoundFile.write(out_path, out);

    if(lpipc_buffer_release(LPADC_BUFFER_PATH, (void *)buf) < 0) {
        fprintf(stderr, "Could not release ADC buffer\n");
        return 1;
    }

    printf("Saved ADC buffer to %s\n", out_path);

    return 0;
}
