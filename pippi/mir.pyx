#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport Wavetable

import aubio
import numpy as np
cimport numpy as np

DEFAULT_WINSIZE = 4096
DEFAULT_HOPSIZE = DEFAULT_WINSIZE//2

np.import_array()

cpdef float[:] flatten(SoundBuffer snd):
    return np.asarray(snd.remix(1).frames, dtype='f').flatten()

cpdef Wavetable pitch(SoundBuffer snd, double tolerance=0.8, int winsize=DEFAULT_WINSIZE, int hopsize=DEFAULT_HOPSIZE):
    o = aubio.pitch('yin', winsize, hopsize, snd.samplerate)
    o.set_tolerance(tolerance)

    cdef list pitches = []

    cdef double pos = 0
    cdef double est = 0
    cdef double con = 0
    cdef double last = 0

    cdef float[:] chunk
    cdef float[:] src = flatten(snd)

    while True:
        chunk = src[pos:pos+hopsize]
        if len(chunk) < hopsize:
            break

        est = o(chunk)[0]
        con = o.get_confidence()

        pos += hopsize

        if con < tolerance:
            pitches += [ last ]
        else:
            last = est
            pitches += [ est ]

    return Wavetable(pitches)

