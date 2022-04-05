#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_PULSEAUDIO
#define MA_NO_ENCODING
#define MA_NO_DECODING
#include "miniaudio/miniaudio.h"
#include "pippi.h"
#include "renderer.h"

#define CHANNELS 2
#define SAMPLERATE 48000


void astrid_mixer(ma_device * device, void * pOut, const void * in, ma_uint32 count) {
    ma_uint32 i;
    float * out;
    int c, frames;
    lpscheduler_t * s;

    s = (lpscheduler_t *)device->pUserData;
    frames = count / CHANNELS;
    out = (float *)pOut;

    for(i=0; i < frames; i++) {
        LPScheduler.tick(s);
        /*ma_data_source_read_pcm_frames(s->current_frame, out, 1, NULL);*/
        for(c=0; c < CHANNELS; c++) {
            *out++ = (float)s->current_frame[c];
        }
    }
    
}

int lprendernode_init(int argc, char * argv[]) {
    PyObject * pmodule;
    wchar_t * program;

    lpbuffer_t * out;
    lpscheduler_t * s;

    size_t length;
    int channels;
    int samplerate;

    s = LPScheduler.create(CHANNELS);
    length = 0;

    Py_SetPath(L"/home/hecanjog/.pyenv/versions/3.9.9/lib/python39.zip:/home/hecanjog/.pyenv/versions/3.9.9/lib/python3.9:/home/hecanjog/.pyenv/versions/3.9.9/lib/python3.9/lib-dynload:/home/hecanjog/.local/lib/python3.9/site-packages:/home/hecanjog/.pyenv/versions/3.9.9/lib/python3.9/site-packages:/home/hecanjog/code/pippi:/home/hecanjog/.pyenv/versions/3.9.9/lib/python3.9/site-packages/PySoundFile-0.9.0.post1-py3.9.egg");

    program = Py_DecodeLocale(argv[0], NULL);
    if(program == NULL) {
        fprintf(stderr, "Fatal error: cannot decode argv[0], got %d arguments\n", argc);
        exit(1);
    }

    if(PyImport_AppendInittab("renderer", PyInit_renderer) == -1) {
        fprintf(stderr, "Error: could not extend in-built modules table for renderer\n");
        exit(1);
    }

    Py_SetProgramName(program);
    Py_Initialize();

    /*printf("Renderer embedded python path: %ls\n", Py_GetPath());*/

    pmodule = PyImport_ImportModule("renderer");
    if(!pmodule) {
        PyErr_Print();
        fprintf(stderr, "Error: could not import cython renderer module\n");
        goto exit_with_error;
    }

    /* Configure miniaudio for playback mode */
    ma_device_config audioconfig = ma_device_config_init(ma_device_type_playback);
    audioconfig.playback.format = ma_format_f32;
    audioconfig.playback.channels = CHANNELS;
    audioconfig.sampleRate = SAMPLERATE;
    audioconfig.dataCallback = astrid_mixer;
    audioconfig.pUserData = s;

    /* init playback device */
    ma_device playback;
    if(ma_device_init(NULL, &audioconfig, &playback) != MA_SUCCESS) {
        fprintf(stderr, "Runtime Error while attempting to configure miniaudio for playback\n");
        goto exit_with_error;
    }

    if(astrid_load_instrument() < 0) {
        PyErr_Print();
        fprintf(stderr, "Runtime Error while attempting to load astrid instrument\n");
        goto exit_with_error;
    }

    if(astrid_render_event() < 0) {
        PyErr_Print();
        fprintf(stderr, "Runtime Error while rendering event from astrid instrument\n");
        goto exit_with_error;
    }

    if(astrid_get_info(&length, &channels, &samplerate) < 0) {
        PyErr_Print();
        fprintf(stderr, "Runtime Error while getting info about the last rendered event\n");
        goto exit_with_error;
    }

    out = LPBuffer.create(length, CHANNELS, SAMPLERATE);
    if(astrid_copy_buffer(out) < 0) {
        PyErr_Print();
        fprintf(stderr, "Runtime Error while copying render data\n");
        goto exit_with_error;
    }
    LPScheduler.schedule_event(s, out, 0);

    /* start playback device */
    ma_device_start(&playback);

    while(LPScheduler.is_playing(s)) {
        usleep((useconds_t)1000);
    }

    LPBuffer.destroy(out);

    ma_device_uninit(&playback);
    PyMem_RawFree(program);
    Py_Finalize();
    return 0;

exit_with_error:
    ma_device_uninit(&playback);
    PyMem_RawFree(program);
    Py_Finalize();
    return 1;
}
