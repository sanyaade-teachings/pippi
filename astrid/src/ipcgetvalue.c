#include "astrid.h"

int main() {
    size_t value;
    void * valuep = &value;

    if(lpipc_getvalue("/tmp/ipcvalue-test", &valuep) < 0) {
        fprintf(stderr, "Could not get value\n");
        return 1;
    }

    value = *((size_t *)valuep);

    if(lpipc_releasevalue("/tmp/ipcvalue-test") < 0) {
        fprintf(stderr, "Could not release value\n");
        return 1;
    }

    printf("Value: %ld\n", value);

    return 0;
}
