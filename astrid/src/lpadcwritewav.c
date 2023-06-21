#include "astrid.h"

int main(int argc, char * argv[]) {
    size_t i;
    lpbuffer_t * snd;
    float * block;
    int adc_shmid;

    if(argc != 3) {
        printf("Usage: %s <adc_shmid:int> <soundfile.wav:str> (%d)\n", argv[0], argc);
    }

    adc_shmid = atoi(argv[1]);
    snd = LPSoundFile.read(argv[2]);

    block = (float *)calloc(snd->length * snd->channels, sizeof(float));
    for(i=0; i < snd->length * snd->channels; i++) {
        block[i] = snd->data[i];
    }

    lpadc_write_block(block, snd->length * snd->channels, adc_shmid);

    printf("Wrote WAV to ADC buffer\n");

    return 0;
}
