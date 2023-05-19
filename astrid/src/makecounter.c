#include "astrid.h"

int main() {
    int semid, shmid;
    size_t * counter;
    union semun arg;

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

    return 0;

exit_with_error:
    syslog(LOG_ERR, "Exited with error\n");
    return 1;
}
