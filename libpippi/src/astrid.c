#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>

#define MINIAUDIO_IMPLEMENTATION
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_JACK
#define MA_NO_ENCODING
#define MA_NO_DECODING
#include "miniaudio/miniaudio.h"

#include "pippi.h"
#include "renderer.h"
#include "astrid.h"

#define CHANNELS 2
#define SAMPLERATE 48000

#define STATUS_PLAYING 1
#define STATUS_PAUSED 0

static volatile int astrid_is_running = 1;
static volatile int astrid_is_playing = 1;
static volatile int astrid_is_looping = 1;

int astrid_channels = 2;

void handle_shutdown(int) {
    astrid_is_running = 0;
}

void handle_play_trigger(int) {
    astrid_is_playing = 1;
}

void miniaudio_callback(ma_device * device, void * pOut, const void * pIn, ma_uint32 count) {
    ma_uint32 i;
    float * out;
    int c;
    lpastridctx_t * ctx;

    ctx = (lpastridctx_t *)device->pUserData;
    out = (float *)pOut;

    for(i=0; i < count; i++) {
        LPScheduler.tick(ctx->s);
        for(c=0; c < astrid_channels; c++) {
            *out++ = (float)ctx->s->current_frame[c];
        }
    }
}

int lprendernode_init() {
    PyObject * pmodule;
    struct sigaction shutdown_action;
    struct sigaction play_action;

    lpbuffer_t * out;
    lpastridctx_t * ctx;

    char * astrid_pythonpath;
    wchar_t * astrid_pythonpath_wchar;
    size_t astrid_pythonpath_length;

    long voice_id;
    size_t length;
    int channels;
    int samplerate;
    int buffer_count;
    char * _astrid_channels;

    _astrid_channels = getenv("ASTRID_CHANNELS");
    if(_astrid_channels != NULL) {
        astrid_channels = atoi(_astrid_channels);
    }
    printf("astrid_channels %d\n", astrid_channels);

    shutdown_action.sa_handler = handle_shutdown;
    sigaction(SIGINT, &shutdown_action, NULL);

    play_action.sa_handler = handle_play_trigger;
    sigaction(SIGUSR1, &play_action, NULL);

    voice_id = (long)syscall(SYS_gettid);

    ctx = (lpastridctx_t*)LPMemoryPool.alloc(1, sizeof(lpastridctx_t));
    ctx->s = LPScheduler.create(astrid_channels);
    length = 0;

    astrid_pythonpath = getenv("ASTRID_PYTHONPATH");
    astrid_pythonpath_length = mbstowcs(NULL, astrid_pythonpath, 0);
    astrid_pythonpath_wchar = calloc(astrid_pythonpath_length+1, sizeof(*astrid_pythonpath_wchar));
    if(astrid_pythonpath_wchar == NULL) {
        fprintf(stderr, "Error: could not allocate memory for wide char path\n");
        exit(1);
    }

    if(mbstowcs(astrid_pythonpath_wchar, astrid_pythonpath, astrid_pythonpath_length) == (size_t) -1) {
        fprintf(stderr, "Error: Could not convert path to wchar_t\n");
        exit(1);
    }

    Py_SetPath(astrid_pythonpath_wchar);

    if(PyImport_AppendInittab("renderer", PyInit_renderer) == -1) {
        fprintf(stderr, "Error: could not extend in-built modules table for renderer\n");
        exit(1);
    }

    Py_SetProgramName(L"astrid");
    Py_Initialize();

    printf("Renderer embedded python path: %ls\n", Py_GetPath());

    pmodule = PyImport_ImportModule("renderer");
    if(!pmodule) {
        PyErr_Print();
        fprintf(stderr, "Error: could not import cython renderer module\n");
        goto exit_with_error;
    }

    /* Configure miniaudio for playback mode */
    ma_device_config audioconfig = ma_device_config_init(ma_device_type_duplex);
    audioconfig.playback.format = ma_format_f32;
    audioconfig.playback.channels = astrid_channels;
    audioconfig.sampleRate = SAMPLERATE;
    audioconfig.dataCallback = miniaudio_callback;
    audioconfig.pUserData = ctx;

    /* init playback device */
    ma_device playback;
    if(ma_device_init(NULL, &audioconfig, &playback) != MA_SUCCESS) {
        fprintf(stderr, "Runtime Error while attempting to configure miniaudio for playback\n");
        goto exit_with_error;
    }

    /* start playback device */
    ma_device_start(&playback);

    if(astrid_load_instrument() < 0) {
        PyErr_Print();
        fprintf(stderr, "Runtime Error while attempting to load astrid instrument\n");
        goto exit_with_error;
    }

    buffer_count = 0;
    while(astrid_is_running) {
        if(astrid_get_messages() < 0) {
            PyErr_Print();
            fprintf(stderr, "Runtime Error while rendering event from astrid instrument\n");
            goto exit_with_error;
        }

        if(astrid_get_instrument_loop_status(&astrid_is_looping) < 0) {
            PyErr_Print();
            fprintf(stderr, "Runtime Error while checking astrid instrument status\n");
            goto exit_with_error;
        }

        if(astrid_is_playing) {
            if(astrid_render_event() < 0) {
                PyErr_Print();
                fprintf(stderr, "Runtime Error while rendering event from astrid instrument\n");
                goto exit_with_error;
            }

            astrid_buffer_count(&buffer_count);
            while(buffer_count > 0) {
                if(astrid_get_info(&length, &channels, &samplerate) < 0) {
                    PyErr_Print();
                    fprintf(stderr, "Runtime Error while getting info about the last rendered event\n");
                    goto exit_with_error;
                }

                out = LPBuffer.create(length, astrid_channels, SAMPLERATE);
                if(astrid_copy_buffer(out) < 0) {
                    PyErr_Print();
                    fprintf(stderr, "Runtime Error while copying render data\n");
                    goto exit_with_error;
                }
                LPScheduler.schedule_event(ctx->s, out, 0);
                astrid_buffer_count(&buffer_count);
            }
        }

        while(LPScheduler.is_playing(ctx->s)) {
            if(astrid_get_messages() < 0) {
                PyErr_Print();
                fprintf(stderr, "Runtime Error while rendering event from astrid instrument\n");
                goto exit_with_error;
            }
            usleep((useconds_t)1000);
        }

        astrid_is_playing = astrid_is_looping;

        usleep((useconds_t)1000);

        /* Free memory for old buffers and events */
        /*LPScheduler.empty(s);*/
        if(astrid_reload_instrument() < 0) {
            PyErr_Print();
            fprintf(stderr, "Runtime Error while attempting to reload astrid instrument\n");
            goto exit_with_error;
        }
    }

    free(astrid_pythonpath_wchar);
    ma_device_uninit(&playback);
    Py_Finalize();
    return 0;

exit_with_error:
    ma_device_uninit(&playback);
    Py_Finalize();
    return 1;
}
