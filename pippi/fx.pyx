# cython: language_level=3

import numpy as np
import numbers
import random
cimport cython
from .soundbuffer cimport SoundBuffer
from . cimport wavetables

cdef double MINDENSITY = 0.001

cpdef SoundBuffer go(SoundBuffer snd, 
                          object factor,
                          double density=1, 
                          double wet=1,
                          double minlength=0.01, 
                          double maxlength=0.06, 
                          double minclip=0.4, 
                          double maxclip=0.8, 
                          object win=None
                    ):
    if wet <= 0:
        return snd

    cdef wavetables.Wavetable factors = None
    if not isinstance(factor, numbers.Real):
        factors = wavetables.Wavetable(factor)

    density = max(MINDENSITY, density)

    cdef double outlen = snd.dur + maxlength
    cdef SoundBuffer out = SoundBuffer(length=outlen, channels=snd.channels, samplerate=snd.samplerate)
    cdef wavetables.Wavetable window
    if win is None:
        window = wavetables.Wavetable(initwin=wavetables.HANN)
    else:
        window = wavetables.Wavetable(win)

    cdef double grainlength = random.triangular(minlength, maxlength)
    cdef double pos = 0
    cdef double clip
    cdef SoundBuffer grain

    while pos < outlen:
        grain = snd.cut(pos, grainlength)
        clip = random.triangular(minclip, maxclip)
        grain *= random.triangular(0, factor * wet)
        grain = grain.clip(-clip, clip)
        out.dub(grain * window.data, pos)

        pos += (grainlength/2) * (1/density)
        grainlength = random.triangular(minlength, maxlength)

    if wet > 0:
        out *= wet

    if wet < 1:
        out.dub(snd * abs(wet-1), 0)

    return out
