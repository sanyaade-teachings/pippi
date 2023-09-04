#include "astrid.h"

int main(int argc, char * argv[]) {
    size_t i;
    lpbuffer_t * snd;
    float * block;

    if(argc != 2) {
        printf("Usage: %s <soundfile.wav:str> (%d)\n", argv[0], argc);
    }

    snd = LPSoundFile.read(argv[1]);

    block = (float *)calloc(snd->length * snd->channels, sizeof(float));
    for(i=0; i < snd->length * snd->channels; i++) {
        block[i] = snd->data[i];
    }

    lpadc_write_block(block, snd->length * snd->channels);

    printf("Wrote WAV to ADC buffer\n");

    return 0;
}
