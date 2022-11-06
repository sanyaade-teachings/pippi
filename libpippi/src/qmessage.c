#include <stdio.h>
#include <stdlib.h>
#include <errno.h> /* errno */
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/stat.h> /* umask */
#include <string.h> /* strlen */

#define MAXMSG 4096

const char SPACE = ' ';

struct response {
    int start;
    char msg[MAXMSG];
};

int main(int argc, char * argv[]) {
    int qfd, count, bytesread, a, i, length;
    struct response resp = {0};

    umask(0);
    if(mkfifo("/tmp/astridq", S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        fprintf(stderr, "Error creating named pipe\n");
        return 1;
    }

    qfd = open("/tmp/astridq", O_WRONLY);

    for(a=0; a < MAXMSG; a++) {
        resp.msg[a] = 0;
    }

    for(a=1; a < argc; a++) {
        printf("Copying '%s' to msg out...\n", argv[a]);
        length = strlen(argv[a]);
        printf("length: %d\n", length);
        for(i=0; i < length; i++) {
            printf("%c", argv[a][i]);
            resp.msg[bytesread] = argv[a][i];
            bytesread++;
        }
        resp.msg[bytesread] = SPACE;
        bytesread++;
        printf("\n...done\n");
    }

    resp.start = bytesread;
    if(write(qfd, &resp, sizeof(struct response)) != sizeof(struct response)) {
        fprintf(stderr, "Error writing, but I guess that is OK...\n");
        return 1;
    }

    printf("sent msgbytes: %d\n", bytesread);

    if(close(qfd) == -1) {
        fprintf(stderr, "Error closing q...\n");
        return 1; 
    }

    return 0;
}


