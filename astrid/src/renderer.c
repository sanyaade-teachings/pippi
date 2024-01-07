#include <signal.h>
#include <sys/syscall.h>

#include "cyrenderer.h"
#include "astrid.h"


static volatile int astrid_is_running = 1;
int astrid_channels = 2;
char * instrument_fullpath; /* eg ../orc/ding.py */
char * instrument_basename; /* eg ding           */

void handle_shutdown(int sig __attribute__((unused))) {
    lpmsg_t msg = {0};
    syslog(LOG_INFO, "Sending shutdown to renderer (SIG %d)...\n", sig);
    msg.type = LPMSG_SHUTDOWN;
    memcpy(&msg.instrument_name, instrument_basename, LPMAXNAME);
    send_play_message(msg);
}

int main(int argc, char * argv[]) {
    struct sigaction shutdown_action;
    char * _instrument_fullpath; 
    char * _instrument_basename; 
    size_t instrument_name_length;
    PyObject * pmodule;
    PyConfig config;
    PyStatus status;

    if(argc != 3) {
        syslog(LOG_ERR, "Error: invalid number of arguments to astrid renderer\n");
        exit(1);
    }

    openlog("astrid-renderer", LOG_PID, LOG_USER);

    mqd_t playqd = -1;
    lpmsg_t msg = {0};

    syslog(LOG_INFO, "Starting renderer...\n");

    /* Setup sigint handler for graceful shutdown */
    shutdown_action.sa_handler = handle_shutdown;
    sigemptyset(&shutdown_action.sa_mask);
    shutdown_action.sa_flags = SA_RESTART; /* Prevent open, read, write etc from EINTR */

    if(sigaction(SIGINT, &shutdown_action, NULL) == -1) {
        syslog(LOG_ERR, "Could not init SIGINT signal handler.\n");
        exit(1);
    }

    if(sigaction(SIGTERM, &shutdown_action, NULL) == -1) {
        syslog(LOG_ERR, "Could not init SIGTERM signal handler.\n");
        exit(1);
    }

    /* Prepare cyrenderer module for import */
    if(PyImport_AppendInittab("cyrenderer", PyInit_cyrenderer) == -1) {
        syslog(LOG_ERR, "Error: could not extend in-built modules table for renderer\n");
        exit(1);
    }

    /* Init python interpreter */
    PyConfig_InitPythonConfig(&config);

    status = PyConfig_SetString(&config, &config.program_name, L"astrid-renderer");
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        return 1;
    }

    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        return 1;
    }

    astrid_channels = ASTRID_CHANNELS; // FIXME

    /* Import cyrenderer */
    pmodule = PyImport_ImportModule("cyrenderer");
    if(!pmodule) {
        PyErr_Print();
        syslog(LOG_ERR, "Error: could not import cython renderer module\n");
        goto lprender_cleanup;
    }

    _instrument_fullpath = argv[1];
    _instrument_basename = argv[2];
    instrument_name_length = strlen(_instrument_basename);
    instrument_fullpath = calloc(strlen(_instrument_fullpath)+1, sizeof(char));
    instrument_basename = calloc(instrument_name_length+1, sizeof(char));
    strcpy(instrument_fullpath, _instrument_fullpath);
    strcpy(instrument_basename, _instrument_basename);

    /* Import python instrument module */
    if(astrid_load_instrument(instrument_fullpath) < 0) {
        PyErr_Print();
        syslog(LOG_ERR, "Error while attempting to load astrid instrument\n");
        goto lprender_cleanup;
    }

    syslog(LOG_INFO, "Astrid renderer... is now rendering!\n");

    if((playqd = astrid_playq_open(instrument_basename)) == (mqd_t) -1) {
        syslog(LOG_CRIT, "Could not open playq for instrument %s. Error: %s\n", instrument_basename, strerror(errno));
        goto lprender_cleanup;
    }

    syslog(LOG_DEBUG, "Opened play queue for %s with fd %d\n", instrument_basename, playqd);

    memcpy(msg.instrument_name, instrument_basename, instrument_name_length);

    /* Start rendering! */
    while(astrid_is_running) {
        msg.onset_delay = 0;
        memset(msg.msg, 0, LPMAXMSG);

        /*syslog(LOG_DEBUG, "Waiting for %s playqueue messages...\n", instrument_basename);*/
        if(astrid_playq_read(playqd, &msg) == (mqd_t) -1) {
            syslog(LOG_ERR, "%s renderer: Could not read message from playq. Error: (%d) %s\n", instrument_basename, errno, strerror(errno));
            goto lprender_cleanup;
        }

        switch(msg.type) {
            case LPMSG_PLAY:
                if(astrid_schedule_python_render(&msg) < 0) {
                    PyErr_Print();
                    syslog(LOG_ERR, "CPython error during renderer loop\n");
                    goto lprender_cleanup;
                }
                break;

            case LPMSG_LOAD:
                if(astrid_reload_instrument(instrument_fullpath) < 0) {
                    PyErr_Print();
                    syslog(LOG_ERR, "Error while attempting to load astrid instrument\n");
                    goto lprender_cleanup;
                }
                break;

            case LPMSG_TRIGGER:
                if(astrid_schedule_python_triggers(&msg) < 0) {
                    PyErr_Print();
                    syslog(LOG_ERR, "CPython error during trigger planning loop\n");
                }
                continue;

            case LPMSG_SHUTDOWN:
                syslog(LOG_DEBUG, "Renderer got %s shutdown message:\n", msg.instrument_name);
                syslog(LOG_DEBUG, "             %d (msg.voice_id)\n", (int)msg.voice_id);
                syslog(LOG_DEBUG, "             %d (msg.type)\n", (int)msg.type);

                astrid_is_running = 0;
                break;

            default:
                continue;
        }

        /* astrid_begin_python_stream(&msg) */
    }

lprender_cleanup:
    syslog(LOG_INFO, "Astrid renderer shutting down...\n");
    Py_Finalize();
    if(playqd != (mqd_t) -1) astrid_playq_close(playqd);
    closelog();
    return 0;
}


