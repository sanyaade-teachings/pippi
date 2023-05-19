#include "astrid.h"

int main(int argc, char * argv[]) {
    lpcounter_t c;

    c.semid = atoi(argv[1]);
    c.shmid = atoi(argv[2]);

    if(lpcounter_destroy(&c) > 0) {
        perror("lpcounter_create");
        return 1;
    }

    return 0;
}
