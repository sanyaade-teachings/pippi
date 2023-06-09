#include "astrid.h"

int main(int argc, char * argv[]) {
    size_t value = 42;
    void * valuep = &value;

    if(argc == 2) value = (size_t)atoi(argv[1]);

    if(lpipc_setvalue("/tmp/ipcvalue-test", valuep, sizeof(size_t)) < 0) {
        fprintf(stderr, "Could not set value\n");
        return 1;
    }

    return 0;
}
