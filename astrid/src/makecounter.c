#include "astrid.h"

int main() {
    int semid, shmid, i;
    size_t * counter;
    union semun arg, dummy;
    struct sembuf sop;
    struct timespec timer;

    openlog("astrid-makecounter", LOG_PID, LOG_USER);

    /* Create the shared memory space for the counter value */
    shmid = shmget(IPC_PRIVATE, sizeof(size_t), IPC_CREAT | 0600);
    if (shmid < 0) {
        perror("shmget");
        goto exit_with_error;    
    }

    /* Create the semaphore used as a read/write lock on the counter */
    semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (semid < 0) {
        perror("semget");
        goto exit_with_error;    
    }

    /* Attach the shared memory for reading and writing */
    counter = shmat(shmid, NULL, 0);
    if (counter == (void*)-1) {
        perror("shmat");
        goto exit_with_error;    
    }

    /* Start the voice IDs at 1 */
    *counter = 1;

    /* Prime the lock by initalizing the sempahore to 1 */
    arg.val = 1;
    if(semctl(semid, 0, SETVAL, arg) < 0) {
        perror("semctl");
        goto exit_with_error;    
    }

    /* Print the IDs for testing */
    printf("shmid = %d\n", shmid);
    printf("semid = %d\n", semid);

    for(i=0; i < 10; i++) {
        /* Print the current Voice ID */
        printf("Voice ID: %d\n", (int)(*counter));

        /* Wait for zero */
        sop.sem_num = 0;
        sop.sem_op = 0;
        sop.sem_flg = 0;
        printf("Waiting for zero...\n");
        if(semop(semid, &sop, 1) < 0) {
            perror("semop");
            goto exit_with_error;
        }

        printf("Zero!\n");
        clock_gettime(CLOCK_REALTIME, &timer);
        lptimeit_since(&timer);

        /* Aquire the lock */
        sop.sem_num = 0;
        sop.sem_op = -1;
        sop.sem_flg = 0;
        printf("Aquiring lock...");
        lptimeit_since(&timer);
        if(semop(semid, &sop, 1) < 0) {
            perror("semop");
            goto exit_with_error;
        }

        printf("Incrementing the counter...\n");
        lptimeit_since(&timer);
        *counter += 1;

        /* Release the lock */
        sop.sem_num = 0;
        sop.sem_op = 1;
        sop.sem_flg = 0;
        printf("Releasing the lock...\n");
        lptimeit_since(&timer);
        if(semop(semid, &sop, 1) < 0) {
            perror("semop");
            goto exit_with_error;
        }

        printf("Done incrementing to zero\n");
        lptimeit_since(&timer);

    }

    lptimeit_since(&timer);
    printf("Cleanup!");

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
