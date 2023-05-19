#include "astrid.h"

int main(int argc, char * argv[]) {
    int semid, shmid;
    struct sembuf sop;
    size_t * counter;
    struct timespec timer;

    openlog("astrid-getvoiceid", LOG_PID, LOG_USER);

    semid = atoi(argv[1]);
    shmid = atoi(argv[2]);

    /* Aquire lock */
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    printf("Aquiring lock...");
    clock_gettime(CLOCK_REALTIME, &timer);
    lptimeit_since(&timer);
    if(semop(semid, &sop, 1) < 0) {
        perror("semop");
        goto exit_with_error;    
    }

    /* Read the current voice ID */
    counter = shmat(shmid, NULL, SHM_RDONLY);
    printf("Voice ID: %d\n", (int)(*counter));
    lptimeit_since(&timer);

    /* Release lock */
    sop.sem_num = 0;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    printf("Releasing the lock...\n");
    lptimeit_since(&timer);
    if(semop(semid, &sop, 1) < 0) {
        perror("semop");
        goto exit_with_error;    
    }

    return 0;

exit_with_error:
    syslog(LOG_ERR, "Exited with error\n");
    return 1;
}
