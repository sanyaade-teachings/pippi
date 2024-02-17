#include "astrid.h"

#define MSGBUFFER_SIZE 100


static volatile int serial_listener_is_running = 1;

void handle_shutdown(__attribute__((unused)) int sig) {
    serial_listener_is_running = 0;
}


int main() {
    char * token;
    char * fakeargv[LPMAXMSG+1];
    char msgstr[] = "k pulsar foo = bar";
    char * tokencache;
    int i;
    lpmsg_t msg = {0};

    openlog("astrid-msgarg", LOG_PID, LOG_USER);

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
 
    printf("msgstr: %s (char1: %u)\n", msgstr, msgstr[0]);

    printf("Converting to argv format\n");

    fakeargv[0] = strtok_r(msgstr, " ", &tokencache);
    for(i=1; ; i++) {
        token = strtok_r(NULL, " ", &tokencache);
        if(token == NULL) break;
        fakeargv[i] = token;
    }
    fakeargv[i] = NULL;

    printf("i: %d\n", i);

    if(parse_message_from_args(i, -1, fakeargv, &msg) < 0) {
        fprintf(stderr, "astrid-msg: Could not parse message from args\n");
        return 1;
    }

    printf("message:\t%s\n", msg.instrument_name);
    printf("\t%d (msg.voice_id)\n", (int)msg.voice_id);
    printf("\t%d (msg.type)\n", (int)msg.type);

    return 0;
}


