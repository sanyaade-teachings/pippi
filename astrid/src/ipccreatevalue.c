#include "astrid.h"

int main() {
    if(lpipc_createvalue("/tmp/ipcvalue-test", sizeof(size_t)) < 0) {
        fprintf(stderr, "Could not create value\n");
        return 1;
    }

    if(lpipc_releasevalue("/tmp/ipcvalue-test") < 0) {
        fprintf(stderr, "Could not release value\n");
        return 1;
    }

    return 0;
}
