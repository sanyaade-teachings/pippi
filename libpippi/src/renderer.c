#include <signal.h>
#include <sys/syscall.h>

#include "pippi.h"
#include "cyrenderer.h"
#include "astrid.h"


static volatile int astrid_is_running = 1;
int astrid_channels = 2;
char * instrument_fullpath; /* eg ../orc/ding.py */
char * instrument_basename; /* eg ding           */

void handle_shutdown(int) {
    astrid_is_running = 0;
}

int main() {
    struct sigaction shutdown_action;

    char * _instrument_fullpath; 
    char * _instrument_basename; 
    size_t instrument_name_length;
    char * astrid_pythonpath_env;
    size_t astrid_pythonpath_length;
    wchar_t * python_path;
    size_t msglength = LPMAXMSG;

    char * _astrid_channels;
    lpastridctx_t * ctx;
    PyObject * pmodule;

    lpmsg_t msg = {0};

    printf("Starting renderer...\n");

    /* Setup sigint handler for graceful shutdown */
    shutdown_action.sa_handler = handle_shutdown;
    sigemptyset(&shutdown_action.sa_mask);
    shutdown_action.sa_flags = 0;
    if(sigaction(SIGINT, &shutdown_action, NULL) == -1) {
        fprintf(stderr, "Could not init SIGINT signal handler.\n");
        exit(1);
    }

    if(sigaction(SIGTERM, &shutdown_action, NULL) == -1) {
        fprintf(stderr, "Could not init SIGTERM signal handler.\n");
        exit(1);
    }

    /* Get python path from env */
    astrid_pythonpath_env = getenv("ASTRID_PYTHONPATH");
    astrid_pythonpath_length = mbstowcs(NULL, astrid_pythonpath_env, 0);
    python_path = calloc(astrid_pythonpath_length+1, sizeof(*python_path));
    if(python_path == NULL) {
        fprintf(stderr, "Error: could not allocate memory for wide char path\n");
        exit(1);
    }

    if(mbstowcs(python_path, astrid_pythonpath_env, astrid_pythonpath_length) == (size_t) -1) {
        fprintf(stderr, "Error: Could not convert path to wchar_t\n");
        exit(1);
    }

    /* Set channels from env */
    _astrid_channels = getenv("ASTRID_CHANNELS");
    if(_astrid_channels != NULL) {
        astrid_channels = atoi(_astrid_channels);
    }
    astrid_channels = 2;

    /* Setup context */
    ctx = (lpastridctx_t*)LPMemoryPool.alloc(1, sizeof(lpastridctx_t));
    ctx->channels = astrid_channels;
    ctx->samplerate = ASTRID_SAMPLERATE;
    ctx->is_playing = 1;
    ctx->is_looping = 1;
    ctx->voice_index = -1;

    /* TODO some human readable ID is useful too? */
    ctx->voice_id = (long)syscall(SYS_gettid);

    /* Set python path */
    /*printf("Setting renderer embedded python path: %ls\n", python_path);*/
    Py_SetPath(python_path);

    /* Prepare cyrenderer module for import */
    if(PyImport_AppendInittab("cyrenderer", PyInit_cyrenderer) == -1) {
        fprintf(stderr, "Error: could not extend in-built modules table for renderer\n");
        exit(1);
    }

    /* Set python program name */
    Py_SetProgramName(L"astrid-renderer");

    /* Init python interpreter */
    Py_InitializeEx(0);

    /* Check renderer python path */
    /*printf("Renderer embedded python path: %ls\n", Py_GetPath());*/

    /* Import cyrenderer */
    pmodule = PyImport_ImportModule("cyrenderer");
    if(!pmodule) {
        PyErr_Print();
        fprintf(stderr, "Error: could not import cython renderer module\n");
        goto lprender_cleanup;
    }

    /* Import python instrument module */
    if(astrid_load_instrument() < 0) {
        PyErr_Print();
        fprintf(stderr, "Error while attempting to load astrid instrument\n");
        goto lprender_cleanup;
    }

    _instrument_fullpath = getenv("INSTRUMENT_PATH");
    _instrument_basename = getenv("INSTRUMENT_NAME");
    instrument_name_length = strlen(_instrument_basename);
    instrument_fullpath = calloc(strlen(_instrument_fullpath)+1, sizeof(char));
    instrument_basename = calloc(instrument_name_length+1, sizeof(char));
    strcpy(instrument_fullpath, _instrument_fullpath);
    strcpy(instrument_basename, _instrument_basename);

    printf("Renderer... is now rendering!\n");
    /* Start rendering! */
    while(astrid_is_running) {
        memset(msg.msg, 0, LPMAXMSG);
        if(get_play_message(instrument_basename, &msg) < 0) {
            fprintf(stderr, "Error fetching message during renderer loop\n");
            goto lprender_cleanup;
        }

        msg.timestamp = 0;
        printf("Got play msg: %s\n", msg.msg);

        if(astrid_tick(msg.msg, &msglength, &msg.timestamp) < 0) {
            PyErr_Print();
            fprintf(stderr, "CPython error during renderer loop\n");
            goto lprender_cleanup;
        }
    }

lprender_cleanup:
    fprintf(stderr, "lprenderer cleanup\n");
    Py_Finalize();
    return 0;
}


