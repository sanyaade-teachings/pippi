#include "astrid.h"

int main() {
    printf("Creating ADC buffer...\n");

    if(lpadc_create() < 0) {
        fprintf(stderr, "Could not create adcbuf shared memory. Error: %s", strerror(errno));
        return 1;
    }

    printf("Created ADC buffer!\n");

    return 0;
}
