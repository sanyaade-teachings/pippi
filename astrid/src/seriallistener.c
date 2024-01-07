#include "astrid.h"


static volatile int serial_listener_is_running = 1;

void handle_shutdown(__attribute__((unused)) int sig) {
    serial_listener_is_running = 0;
}


int main(int argc, char * argv[]) {
    int tty;
    ssize_t bytesread;
    char * tty_path;

    lpmsg_t msg = {0};

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
 
    tty = open(tty_path, O_RDONLY);
    if(tty < 0) {
        syslog(LOG_ERR, "Problem connecting to TTY\n");
        exit(1);
    }

    while(serial_listener_is_running) {
        bytesread = read(tty, &msg, sizeof(lpmsg_t));
        if(bytesread != sizeof(lpmsg_t)) {
            syslog(LOG_ERR, "serial listener read %ld bytes on device %s...\n", bytesread, tty_path);
            continue;
        }

        if(msg.type == LPMSG_SHUTDOWN) break;
    }

    close(tty);

    return 0;
}


