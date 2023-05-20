#include "astrid.h"

int main(int argc, char * argv[]) {
    lpcounter_t c;
    int voice_id;

    c.semid = lpipc_getid(LPVOICE_ID_SEMID);
    c.shmid = lpipc_getid(LPVOICE_ID_SHMID);

    if((voice_id = lpcounter_read_and_increment(&c)) < 0) {
        perror("lpcounter_create");
        return 1;
    }

    printf("Voice ID: %d\n", voice_id);

    return 0;
}
