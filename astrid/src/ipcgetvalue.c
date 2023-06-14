#include "astrid.h"

int main() {
    size_t value;
    void * valuep;

    if(lpipc_getvalue("/tmp/ipcvalue-test", &valuep) < 0) {
        fprintf(stderr, "Could not set value\n");
        return 1;
    }

    value = *((size_t *)valuep);
    printf("Value: %ld\n", value);

    return 0;
}
