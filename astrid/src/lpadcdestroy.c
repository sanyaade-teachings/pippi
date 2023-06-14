#include "astrid.h"

int main() {
    printf("Destroying ADC buffer...\n");

    if(lpadc_destroy() < 0) {
        fprintf(stderr, "Could not destroy adcbuf shared memory. Error: %s", strerror(errno));
        return 1;
    }

    printf("Destroyed ADC buffer!\n");

    return 0;
}
