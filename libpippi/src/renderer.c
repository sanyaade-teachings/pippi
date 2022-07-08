#include <fcntl.h>
#include <mqueue.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>

#include <hiredis/hiredis.h>

#include "pippi.h"
#include "cyrenderer.h"
#include "astrid.h"

static volatile int astrid_is_running = 1;
int astrid_channels = 2;
redisContext * redis_ctx;

void handle_shutdown(int) {
    astrid_is_running = 0;
}

void wait_for_play_message() {
    redisReply * redis_reply;
    redis_reply = redisCommand(redis_ctx, "BLPOP astridplays 0");
    if(redis_reply->str != NULL) printf("play message result: %s\n", redis_reply->str); 
    freeReplyObject(redis_reply);
}

int main() {
    struct sigaction shutdown_action;

    char * astrid_pythonpath_env;
    size_t astrid_pythonpath_length;
    wchar_t * python_path;

    char * _astrid_channels;
    lpastridctx_t * ctx;
    PyObject * pmodule;
    struct timeval redis_timeout = {15, 0};

    printf("Starting renderer...\n");

    /* Setup sigint handler for graceful shutdown */
    shutdown_action.sa_handler = handle_shutdown;
    sigaction(SIGINT, &shutdown_action, NULL);

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

    /* Connect to redis */
    redis_ctx = redisConnectWithTimeout("127.0.0.1", 6379, redis_timeout);
    if(redis_ctx == NULL) {
        fprintf(stderr, "Could not start connection to redis.\n");
        exit(1);
    }

    /* Check redis connection */
    if(redis_ctx->err) {
        fprintf(stderr, "There was a problem while connecting to redis. %s\n", redis_ctx->errstr);
        exit(1);
    }

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
    printf("Setting renderer embedded python path: %ls\n", python_path);
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
    printf("Renderer embedded python path: %ls\n", Py_GetPath());

    /* Import cyrenderer */
    pmodule = PyImport_ImportModule("cyrenderer");
    if(!pmodule) {
        PyErr_Print();
        fprintf(stderr, "Error: could not import cython renderer module\n");
        goto lprender_thread_cleanup;
    }

    /* Import python instrument module */
    if(astrid_load_instrument() < 0) {
        PyErr_Print();
        fprintf(stderr, "Error while attempting to load astrid instrument\n");
        goto lprender_thread_cleanup;
    }

    /* Start rendering! */
    while(astrid_is_running) {
        /* First reload the instrument module */
        if(astrid_reload_instrument() < 0) {
            PyErr_Print();
            fprintf(stderr, "Error while attempting to reload astrid instrument\n");
            goto lprender_thread_cleanup;
        }

        /* Wait until a play message arrives on the queue */
        wait_for_play_message();

        /* Prompt the cyrenderer to check for redis messages and 
         * pass them along to the python instrument module */
        if(astrid_get_messages() < 0) {
            PyErr_Print();
            fprintf(stderr, "Error while rendering event from astrid instrument\n");
            goto lprender_thread_cleanup;
        }

        /* Check for an updated LOOP status on the python 
         * instrument module since last reload */
        if(astrid_get_instrument_loop_status(&ctx->is_looping) < 0) {
            PyErr_Print();
            fprintf(stderr, "Error while checking astrid instrument status\n");
            goto lprender_thread_cleanup;
        }

        /* Render that shit! The cyrenderer module will dump 
         * serialized buffers into the redis buffer queue */
        if(astrid_render_event() < 0) {
            PyErr_Print();
            fprintf(stderr, "Error while rendering event from astrid instrument\n");
            goto lprender_thread_cleanup;
        }

        /* TODO rework these vestigial status indicators */
        ctx->is_playing = ctx->is_looping;
    }

lprender_thread_cleanup:
    Py_Finalize();
    pthread_exit(0);
    return 0;
}


