#include "astrid.h"

#define MSGBUFFER_SIZE 100


static volatile int serial_listener_is_running = 1;

void handle_shutdown(__attribute__((unused)) int sig) {
    serial_listener_is_running = 0;
}


int main(int argc, char * argv[]) {
    int tty;
    ssize_t bytesread;
    char * tty_path, * token, * tokencache;
    struct termios options;
    lpmsg_t msg = {0};
    char msgstr[MSGBUFFER_SIZE];
    char * fakeargv[LPMAXMSG+1];


    openlog("astrid-seriallistener", LOG_PID, LOG_USER);

    if(argc != 2) {
        printf("Usage: %s </dev/ttyUSB0> (%d)\n", argv[0], argc);
    }

    tty_path = argv[1];


    /* setup signal handlers */
    struct sigaction shutdown_action;
    shutdown_action.sa_handler = handle_shutdown;
    sigemptyset(&shutdown_action.sa_mask);
    shutdown_action.sa_flags = SA_RESTART; /* Prevent open, read, write etc from EINTR */

    /* Keyboard interrupt triggers cleanup and shutdown */
    if(sigaction(SIGINT, &shutdown_action, NULL) == -1) {
        syslog(LOG_ERR, "Could not init SIGINT signal handler\n");
        exit(1);
    }

    /* Terminate signals also trigger cleanup and shutdown */
    if(sigaction(SIGTERM, &shutdown_action, NULL) == -1) {
        syslog(LOG_ERR, "Could not init SIGTERM signal handler\n");
        exit(1);
    }
 
    printf("Opening tty...\n");
    /* open tty */
    tty = open(tty_path, O_RDONLY | O_NOCTTY);
    if(tty < 0) {
        syslog(LOG_ERR, "Problem connecting to TTY\n");
        exit(1);
    }

    printf("Setting baud rate...\n");
    /* set baud rate */
    tcgetattr(tty, &options);
    cfsetispeed(&options, B1000000);
    tcsetattr(tty, TCSANOW, &options);

    printf("Waiting for messages...\n");

    while(serial_listener_is_running) {
        printf("Reading messages...\n");
        bytesread = read(tty, &msgstr, MSGBUFFER_SIZE);
        printf("serial listener read %ld bytes on device %s...\n", bytesread, tty_path);
        printf("msgstr: %s (char1: %u)\n", msgstr, msgstr[0]);

        if(bytesread <= 1) {
            memset(msgstr, 0, MSGBUFFER_SIZE);
            continue;
        }

        printf("Converting to argv format\n");
        int i = 0;
        fakeargv[0] = strtok_r(msgstr, " ", &tokencache);

        for(i=1; ; i++) {
            token = strtok_r(NULL, " ", &tokencache);
            fakeargv[i] = token;
            if(token == NULL) break;
        }

        /* -1 offset to skip the usual program name 0 index in real argv */
        if(parse_message_from_args(i, -1, fakeargv, &msg) < 0) {
            fprintf(stderr, "astrid-msg: Could not parse message from args\n");
            return 1;
        }

        memset(msgstr, 0, MSGBUFFER_SIZE);

        printf("message:\t%s\n", msg.instrument_name);
        printf("\t%d (msg.voice_id)\n", (int)msg.voice_id);
        printf("\t%d (msg.type)\n", (int)msg.type);

        if(send_play_message(msg) < 0) {
            fprintf(stderr, "astrid-msg: Could not send play message...\n");
            return 1;
        }
    }

    close(tty);

    return 0;
}


