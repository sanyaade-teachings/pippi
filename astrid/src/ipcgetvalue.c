#include "astrid.h"

int main() {
    size_t value;
    void * valuep;

    if(lpipc_getvalue(LPADC_WRITEPOS_PATH, &valuep) < 0) {
        fprintf(stderr, "Could not set value\n");
        return 1;
    }

    value = *((size_t *)valuep);

    if(lpipc_releasevalue(LPADC_WRITEPOS_PATH) < 0) {
        fprintf(stderr, "Could not release value\n");
        return 1;
    }

    printf("Value: %ld\n", value);

    return 0;
}
