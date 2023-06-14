#include "astrid.h"

int main(int argc, char * argv[]) {
    if(lpipc_destroyvalue("/tmp/ipcvalue-test") < 0) {
        fprintf(stderr, "Could not destroy value\n");
        return 1;
    }

    return 0;
}
