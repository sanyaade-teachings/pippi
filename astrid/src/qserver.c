#include <stdio.h>
#include <stdlib.h>
#include <errno.h> /* errno */
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/stat.h> /* umask */

#define MAXMSG 4096
#define TESTMSG "p ding o:3 v:2 foo:bar biz=baz blah blah blah blah\n"

static volatile int is_running = 1;

void handle_shutdown(int sig __attribute__((unused))) {
    is_running = 0;
}

struct response {
    int start;
    char msg[MAXMSG];
};

int main() {
    int qfd, count;
    struct sigaction shutdown_action;
    struct response resp = {
        .start=0,
        .msg=TESTMSG
    };

    printf("Starting qserver...\n");

    /* Setup sigint handler for graceful shutdown */
    shutdown_action.sa_handler = handle_shutdown;
    sigemptyset(&shutdown_action.sa_mask);
    shutdown_action.sa_flags = 0;
    if(sigaction(SIGINT, &shutdown_action, NULL) == -1) {
        fprintf(stderr, "Could not init SIGINT signal handler.\n");
        exit(1);
    }

    if(sigaction(SIGPIPE, &shutdown_action, NULL) == -1) {
        fprintf(stderr, "Could not init SIGPIPE signal handler.\n");
        exit(1);
    }

    umask(0);

    if(mkfifo("/tmp/astridq", S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        fprintf(stderr, "Error creating named pipe\n");
        return 1;
    }

    qfd = open("/tmp/astridq", O_WRONLY);

    printf("Created q, ctl-c to quit\n");

    count = 0;
    while(is_running) {
        resp.start = count;
        if(write(qfd, &resp, sizeof(struct response)) != sizeof(struct response)) {
            fprintf(stderr, "Error writing, but I guess that is OK...\n");
            continue;
        }
        count += 1;
        sleep(2);
    }
    printf("Quitting...\n");

    if(close(qfd) == -1) {
        fprintf(stderr, "Error closing q...\n");
        return 1; 
    }

    return 0;
}


