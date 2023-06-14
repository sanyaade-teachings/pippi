#include "astrid.h"

int main(int argc, char * argv[]) {
    int i;
    double * block;
    lpbuffer_t * out;
    char * out_path;

    if(argc != 2) {
        printf("Usage: %s <outpath.wav> (%d)\n", argv[0], argc);
    }

    out_path = argv[1];

    /*
    block = (double *)calloc(ASTRID_SAMPLERATE * ASTRID_CHANNELS, sizeof(double));
    */

    printf("Reading block of samples...\n");
    if(lpadc_read_block_of_samples(0, ASTRID_SAMPLERATE, &block) < 0) {
        fprintf(stderr, "Could not create adcbuf shared memory. Error: %s", strerror(errno));
        return 1;
    }

    printf("Copying block of samples...\n");
    out = LPBuffer.create(ASTRID_SAMPLERATE, ASTRID_CHANNELS, ASTRID_SAMPLERATE);
    for(i=0; i < ASTRID_SAMPLERATE * ASTRID_CHANNELS; i++) {
        out->data[i] = (lpfloat_t)block[i];
    }

    printf("Writing block of samples...\n");
    LPSoundFile.write(out_path, out);

    printf("Done!\n");

    return 0;
}
