#include "astrid.h"

int main() {
    if(lpipc_destroyvalue("/tmp/ipcvalue-test") < 0) {
        fprintf(stderr, "Could not destroy value\n");
        return 1;
    }

    return 0;
}
