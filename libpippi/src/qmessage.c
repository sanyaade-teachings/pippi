#include <stdio.h>
#include <stdlib.h>
#include <errno.h> /* errno */
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/stat.h> /* umask */
#include <string.h> /* strlen */

#include "astrid.h"

int main(int argc, char * argv[]) {
    int bytesread, a, i, length;
    lpeventctx_t ctx = {0};
    char message_params[LPMAXMSG] = {0};

    printf("msgsize: %d\n", (int)sizeof(lpmsg_t));

    for(a=1; a < argc; a++) {
        length = strlen(argv[a]);
        for(i=0; i < length; i++) {
            message_params[bytesread] = argv[a][i];
            bytesread++;
        }
        message_params[bytesread] = SPACE;
        bytesread++;
    }

    strncpy(ctx.instrument_name, "ding", 5);
    //ctx.instrument_name = &instrument_name; 
    //ctx.play_params = &message_params;
    strncpy(ctx.play_params, message_params, bytesread);

    printf("Sending message %s...\n", ctx.play_params);
    if(send_play_message(&ctx) < 0) {
        fprintf(stderr, "Could not send play message...\n");
        return 1;
    }

    return 0;
}


