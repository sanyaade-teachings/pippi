#cython: language_level=3

from libc.stdlib cimport malloc, free

import numpy as np
cimport numpy as np

from pippi.soundbuffer cimport SoundBuffer
from pippi.tune import ftom

np.import_array()

SAMPLERATE = 44100
CHANNELS = 2

cpdef SoundBuffer render(str font, double length, double freq, double amp, int channels=CHANNELS, int samplerate=SAMPLERATE):
    cdef tsf* TSF = tsf_load_filename(font.encode('UTF-8'))

    # TSF only supports stereo or mono
    if channels == 1:
        tsf_set_output(TSF, TSF_MONO, samplerate, 0)
    else:
        channels = 2
        tsf_set_output(TSF, TSF_STEREO_UNWEAVED, samplerate, 0)

    cdef int _length = <int>(length * samplerate)
    cdef int note = <int>ftom(freq)

    cdef double[:,:] out = np.zeros((_length, channels), dtype='d')
    cdef float* _out = <float*>malloc(sizeof(float) * _length * 2)

    tsf_note_on(TSF, 0, note, amp)
    tsf_render_float(TSF, _out, _length, 0)

    cdef size_t offset = 0
    cdef size_t c = 0

    for c in range(channels):
        offset = c * _length
        for i in range(_length):
            out[i,c] = _out[i+offset]

    free(_out)

    return SoundBuffer(out, channels=1, samplerate=44100)
