#include "astrid.h"
#include "solenoids.h"

int main(int argc, char * argv[]) {
    int soletty;
    ssize_t bytesread;
    char trigger = LPSOLEALL;

    lpmsg_t msg = {0};

    soletty = open("/dev/ttyACM0", O_RDONLY);
    if(soletty < 0) {
        printf("Problem connecting to tty\n");
        exit(1);
    }
    while(1) {
        bytesread = read(soletty, &trigger, 1);
        if(trigger == 10) continue;
        printf("trrrrrigger! %c (bytes read: %d)\n", (char)trigger, (int)bytesread);
        printf("Sending play message\n");
        if(send_play_message(msg) < 0) {
            fprintf(stderr, "Could not send play message...\n");
            return 1;
        }
    }

    close(soletty);

    return 0;
}


