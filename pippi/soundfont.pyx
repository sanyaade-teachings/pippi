#cython: language_level=3

from libc.stdlib cimport malloc, free

import numpy as np
cimport numpy as np

from pippi.soundbuffer cimport SoundBuffer
from pippi.tune import ftom

np.import_array()

SAMPLERATE = 44100
CHANNELS = 2
BLOCKSIZE = 64

cdef double[:,:] render(str font, object events, int voice, int channels, int samplerate):
    if not isinstance(events, list) and not isinstance(events, tuple):
        raise ValueError('Invalid event list of type %s' % type(events))

    cdef tsf* TSF = tsf_load_filename(font.encode('UTF-8'))

    # Total length is last event onset time + length
    cdef double length = events[-1][0] + events[-1][1]
    cdef int _length = <int>(length * samplerate)

    cdef int _voice = max(0, voice-1)

    # TSF only supports stereo or mono
    if channels == 1:
        tsf_set_output(TSF, TSF_MONO, samplerate, 0)
    else:
        channels = 2
        tsf_set_output(TSF, TSF_STEREO_UNWEAVED, samplerate, 0)

    cdef double[:,:] out = np.zeros((_length, channels), dtype='d')
    cdef float* block = <float*>malloc(sizeof(float) * BLOCKSIZE * 2)

    cdef int elapsed = 0
    cdef size_t c = 0
    cdef size_t offset = 0

    cdef list playing = []

    while elapsed < _length:
        offset = 0
        for ei, e in enumerate(events):
            if e[0] * samplerate <= elapsed and ei not in playing:
                print('PLAY', ei, e[0], elapsed)
                _play_note(TSF, _voice, e)
                playing += [ ei ]

        tsf_render_float(TSF, block, BLOCKSIZE, 0)

        for c in range(channels):
            offset = c * BLOCKSIZE
            for i in range(BLOCKSIZE):
                if i+elapsed < _length:
                    out[i+elapsed,c] = block[i+offset]

        elapsed += BLOCKSIZE

    free(block)

    return out

cdef _play_note(tsf* TSF, int _voice, tuple event):
    cdef int note = <int>ftom(event[2])
    tsf_note_on(TSF, _voice, note, event[3])

cpdef SoundBuffer play(str font, double length=1, double freq=440, double amp=1, int voice=1, int channels=CHANNELS, int samplerate=SAMPLERATE):
    cdef list events = [(0, length, freq, amp)]
    return SoundBuffer(render(font, events, voice, channels, samplerate), channels=channels, samplerate=samplerate)

cpdef SoundBuffer playall(str font, object events, int voice=1, int channels=CHANNELS, int samplerate=SAMPLERATE):
    return SoundBuffer(render(font, events, voice, channels, samplerate), channels=channels, samplerate=samplerate)


