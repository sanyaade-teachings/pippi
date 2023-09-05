#include "astrid.h"


static volatile int serial_listener_is_running = 1;

void handle_shutdown(__attribute__((unused)) int sig) {
    serial_listener_is_running = 0;
}


int main(int argc, char * argv[]) {
    int tty;
    ssize_t bytesread;
    char * tty_path;

    lpserial_param_t param = {0};

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
        bytesread = read(tty, &param, sizeof(lpserial_param_t));
        if(bytesread != sizeof(lpserial_param_t)) {
            syslog(LOG_ERR, "serial listener read %ld bytes on device %s...\n", bytesread, tty_path);
            continue;
        }

        if(param.type == LPSERIAL_PARAM_SHUTDOWN) break;

        /*
        if (param.type == LPSERIAL_PARAM_BOOL) {
            if(lpserial_setbool(param.device, param.id, param.value.as_int) < 0) {
                syslog(LOG_ERR, "Could not set ctl value...\n");
            }
        } else if (param.type == LPSERIAL_PARAM_CTL) {
            if(lpserial_setctl(param.device, param.id, param.value.as_lpfloat_t) < 0) {
                syslog(LOG_ERR, "Could not set ctl value...\n");
            }
        } else if (param.type == LPSERIAL_PARAM_SIG) {
            if(lpserial_setsig(param.device, param.id, param.value.as_lpfloat_t) < 0) {
                syslog(LOG_ERR, "Could not set ctl value...\n");
            }
        } else if (param.type == LPSERIAL_PARAM_NUM) {
            if(lpserial_setnum(param.device, param.id, param.value.as_size_t) < 0) {
                syslog(LOG_ERR, "Could not set ctl value...\n");
            }
        } else if (param.type == LPSERIAL_PARAM_CC) {
            if(lpmidi_setcc(param.device, param.id, param.value.as_int) < 0) {
                syslog(LOG_ERR, "Could not set cc value...\n");
            }
        } else if(param.type == LPSERIAL_PARAM_NOTEON) {
            if(lpmidi_setnote(param.id, param.value.as_int, param.device) < 0) {
                syslog(LOG_ERR, "Could not set note...\n");
            }

            if(lpmidi_trigger_notemap(param.device, param.id) < 0) {
                syslog(LOG_ERR, "Could not trigger notemap...\n");
            }
        } else if(param.type == LPSERIAL_PARAM_NOTEOFF) {
            if(lpmidi_setnote(param.id, param.value.as_int, param.device) < 0) {
                syslog(LOG_ERR, "Could not set note...\n");
            }
        }
        */
    }

    close(tty);

    return 0;
}


