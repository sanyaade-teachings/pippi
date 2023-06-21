#include "astrid.h"

int main() {
    int adc_shmid;
    printf("Creating ADC buffer...\n");

    if((adc_shmid = lpadc_create()) < 0) {
        fprintf(stderr, "Could not create adcbuf shared memory. Error: %s", strerror(errno));
        return 1;
    }

    printf("Created ADC buffer!\n");
    printf("adc_shmid: %d\n", adc_shmid);

    return 0;
}
