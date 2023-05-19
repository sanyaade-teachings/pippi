#include "astrid.h"

int main() {
    lpcounter_t c;

    if(lpcounter_create(&c) > 0) {
        perror("lpcounter_create");
        return 1;
    }

    /* Print the IDs for testing */
    printf("shmid = %d\n", c.shmid);
    printf("semid = %d\n", c.semid);

    return 0;
}
