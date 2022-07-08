#include <fcntl.h>
#include <mqueue.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>

#include "pippi.h"
#include "cyrenderer.h"
#include "astrid.h"

static volatile int astrid_is_running = 1;
int astrid_channels = 2;

void handle_shutdown(int) {
    astrid_is_running = 0;
}

int main() {
    struct sigaction shutdown_action;

    char * astrid_pythonpath_env;
    size_t astrid_pythonpath_length;
    wchar_t * python_path;

    char * _astrid_channels;
    lpastridctx_t * ctx;
    PyObject * pmodule;

    shutdown_action.sa_handler = handle_shutdown;
    sigaction(SIGINT, &shutdown_action, NULL);

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

    _astrid_channels = getenv("ASTRID_CHANNELS");
    if(_astrid_channels != NULL) {
        astrid_channels = atoi(_astrid_channels);
    }

    ctx = (lpastridctx_t*)LPMemoryPool.alloc(1, sizeof(lpastridctx_t));
    ctx->channels = astrid_channels;
    ctx->samplerate = ASTRID_SAMPLERATE;
    ctx->is_playing = 1;
    ctx->is_looping = 1;
    ctx->voice_index = -1;

    printf("Starting renderer...\n");

    printf("Setting voice id...\n");
    ctx->voice_id = (long)syscall(SYS_gettid);

    printf("Setting python path...\n");
    printf("Renderer embedded python path: %ls\n", python_path);

    Py_SetPath(python_path);

    if(PyImport_AppendInittab("cyrenderer", PyInit_cyrenderer) == -1) {
        fprintf(stderr, "Error: could not extend in-built modules table for renderer\n");
        exit(1);
    }

    printf("Setting program name...\n");
    Py_SetProgramName(L"astrid-renderer");

    printf("init python...\n");
    Py_InitializeEx(0);

    /*printf("Renderer embedded python path: %ls\n", Py_GetPath());*/

    printf("importing renderer...\n");
    pmodule = PyImport_ImportModule("cyrenderer");
    printf("imported renderer...\n");
    if(!pmodule) {
        PyErr_Print();
        fprintf(stderr, "Error: could not import cython renderer module\n");
        goto lprender_thread_cleanup;
    }

    printf("loading instrument...\n");
    if(astrid_load_instrument() < 0) {
        PyErr_Print();
        fprintf(stderr, "Error while attempting to load astrid instrument\n");
        goto lprender_thread_cleanup;
    }

    while(astrid_is_running) {
        printf("Thread %d running...\n", (int)ctx->thread_id);
        if(astrid_get_messages() < 0) {
            PyErr_Print();
            fprintf(stderr, "Error while rendering event from astrid instrument\n");
            goto lprender_thread_cleanup;
        }

        if(astrid_get_instrument_loop_status(&ctx->is_looping) < 0) {
            PyErr_Print();
            fprintf(stderr, "Error while checking astrid instrument status\n");
            goto lprender_thread_cleanup;
        }

        if(astrid_render_event() < 0) {
            PyErr_Print();
            fprintf(stderr, "Error while rendering event from astrid instrument\n");
            goto lprender_thread_cleanup;
        }

        printf("Done rendering...\n");
        ctx->is_playing = ctx->is_looping;

        usleep((useconds_t)1000);

        if(astrid_reload_instrument() < 0) {
            PyErr_Print();
            fprintf(stderr, "Error while attempting to reload astrid instrument\n");
            goto lprender_thread_cleanup;
        }
    }

lprender_thread_cleanup:
    Py_Finalize();
    pthread_exit(0);
    return 0;
}


