#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/stat.h> /* umask */

#define MAXMSG 4096

static volatile int is_running = 1;

struct response {
    int start;
    char msg[MAXMSG];
};

void handle_shutdown(int sig __attribute__((unused))) {
    is_running = 0;
}

int main() {
    int qfd;
    struct sigaction shutdown_action;
    struct response resp;

    printf("Starting reader...\n");

    /* Setup sigint handler for graceful shutdown */
    shutdown_action.sa_handler = handle_shutdown;
    sigemptyset(&shutdown_action.sa_mask);
    shutdown_action.sa_flags = 0;
    if(sigaction(SIGINT, &shutdown_action, NULL) == -1) {
        fprintf(stderr, "Could not init SIGINT signal handler.\n");
        exit(1);
    }

    umask(0);

    qfd = open("/tmp/astridq", O_RDONLY);

    printf("Opened q for reading, ctl-c to quit\n");

    while(is_running) {
        if(read(qfd, &resp, sizeof(struct response)) != sizeof(struct response)) {
            //fprintf(stderr, "Error writing, but I guess that is OK...\n");
            //continue;
            is_running = 0;
            continue;
        }

        printf("The count is: %d\n", resp.start);
        printf("The message is: %s\n", resp.msg);
    }

    printf("Quitting...\n");

    if(close(qfd) == -1) {
        fprintf(stderr, "Error closing q...\n");
        return 1; 
    }

    return 0;
}


