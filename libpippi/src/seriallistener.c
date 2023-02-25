#include "astrid.h"
#include "solenoids.h"

int main(int argc, char * argv[]) {
    int soletty;
    ssize_t bytesread;
    char msg = LPSOLEALL;

    lpeventctx_t ctx = {
        "osc",
        "",
    };
    //char message_params[LPMAXMSG] = {0};
    //char instrument_name[LPMAXNAME] = {0};

    soletty = open("/dev/ttyACM0", O_RDONLY);
    if(soletty < 0) {
        printf("Problem connecting to tty\n");
        exit(1);
    }
    while(1) {
        bytesread = read(soletty, &msg, 1);
        if(msg == 10) continue;
        printf("trrrrrigger! %c (bytes read: %d)\n", (char)msg, (int)bytesread);
        printf("Sending play message\n");
        if(send_play_message(&ctx) < 0) {
            fprintf(stderr, "Could not send play message...\n");
            return 1;
        }
    }

    close(soletty);

    return 0;
}


