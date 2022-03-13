#include "pippi.h"
#include "renderer.h"

/*
lpmsgbus_t * create_msgbus() {
    lpmsgbus_t * msgbus;

    msgbus = LPMemoryPool.alloc(1, sizeof(lpmsgbus_t));

    return msgbus;
}
*/


int lprendernode_init(int argc, char * argv[]) {
    PyObject * pmodule;
    wchar_t * program;
    lpbuffer_t * out;
    size_t length;
    int channels;
    int samplerate;

    length = 0;
    channels = -1;
    samplerate = -1;

    printf("%s\n", getenv("PATH"));

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

    printf("Renderer embedded python path: %ls\n", Py_GetPath());

    pmodule = PyImport_ImportModule("renderer");
    if(!pmodule) {
        PyErr_Print();
        fprintf(stderr, "Error: could not import cython renderer module\n");
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

    out = LPBuffer.create(length, channels, samplerate);
    if(astrid_copy_buffer(out) < 0) {
        PyErr_Print();
        fprintf(stderr, "Runtime Error while copying render data\n");
        goto exit_with_error;
    }

    printf("After after\n");
    printf("Writing buffer\n");
    /*printf("Writing soundbuffer %dx%d\n", (int)out->channels, (int)out->length);*/
    LPSoundFile.write("renders/renderer-out.wav", out);
    printf("Written buffer\n");
    LPBuffer.destroy(out);

    PyMem_RawFree(program);
    Py_Finalize();

    return 0;

exit_with_error:
    PyMem_RawFree(program);
    Py_Finalize();
    return 1;
}
