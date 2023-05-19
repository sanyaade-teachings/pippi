#include "astrid.h"

int main(int argc, char * argv[]) {
    int semid, shmid;
    union semun dummy;

    openlog("astrid-getvoiceid", LOG_PID, LOG_USER);

    semid = atoi(argv[1]);
    shmid = atoi(argv[2]);

    /* Remove the semaphore and shared memory counter */
    if(shmctl(shmid, IPC_RMID, NULL) < 0) {
        perror("shmctl");
        goto exit_with_error;
    }

    if(semctl(semid, 0, IPC_RMID, dummy) < 0) {
        perror("semctl");
        goto exit_with_error;
    }

    return 0;

exit_with_error:
    syslog(LOG_ERR, "Exited with error\n");
    return 1;
}
