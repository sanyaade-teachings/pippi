""" Some shortcuts to doing boilerplate work with SoundBuffers
    which make live coding suck less, hopefully.
"""
import glob
import math
import multiprocessing as mp
import numpy as np
import random

cimport cython

from .soundbuffer import SoundBuffer
from .soundbuffer cimport SoundBuffer
from . cimport grains
cimport wavetables as wts

# Expose some C flags / constants to python
# FIXME might be faster to use newish cpdef enum defs? donno
SINE = wts.SINE
COS = wts.COS
TRI = wts.TRI
SAW = wts.SAW
PHASOR = wts.PHASOR
RSAW = wts.RSAW
HANN = wts.HANN
HAMM = wts.HAMM
BLACK = wts.BLACK
BLACKMAN = wts.BLACK
BART = wts.BART
BARTLETT = wts.BARTLETT
KAISER = wts.KAISER
SQUARE = wts.SQUARE
RND = wts.RND

# Just a shorthand for MS in scripts. 
# For example:
#
# >>> snd.cut(0.75, 32*dsp.MS)
#
# Which cuts, starting from 0.75 seconds in, 
# a 32 millisecond section from the SoundBuffer `snd`.
MS = 0.001

def mix(sounds):
    """ Mix a list of sounds into a new sound
    """
    out = SoundBuffer(length=1)
    for sound in sounds:
        out &= sound

    return out

cpdef SoundBuffer stack(list sounds):
    cdef int channels = 0
    cdef int copied_channels = 0
    cdef int samplerate = sounds[0].samplerate
    cdef double length = 0
    cdef SoundBuffer sound

    for sound in sounds:
        channels += sound.channels
        if sound.dur > length:
            length = sound.dur

    cdef SoundBuffer out = SoundBuffer(length=length, channels=channels, samplerate=samplerate)

    cdef int channel_to_copy = 0
    cdef int current_sound_index = 0
    cdef int source_channel_to_copy = 0

    while copied_channels < channels:
        out.frames.base[:,channel_to_copy] = sounds[current_sound_index].frames.base[:,source_channel_to_copy]    


    return out

def join(sounds, channels=2, samplerate=44100):
    """ Concatenate a list of sounds into a new sound
    """
    out = SoundBuffer(length=1, channels=channels, samplerate=samplerate)
    for sound in sounds:
        out += sound

    return out

def silence(length=-1, channels=2, samplerate=44100):
    """ Create a buffer of silence of a given length
    """
    return SoundBuffer(length=length, channels=channels, samplerate=samplerate)

def buffer(frames=None, length=-1, channels=2, samplerate=44100):
    """ Identical to `silence` -- creates an empty buffer of a given length
    """
    return SoundBuffer(frames=frames, length=length, channels=channels, samplerate=samplerate)

def read(frames, channels=2, samplerate=44100):
    """ Read a soundfile from disk and return a `SoundBuffer` with its contents
        Equiv to `snd = SoundBuffer(filename)`
    """
    if isinstance(frames, str):
        return SoundBuffer(filename=frames, channels=channels, samplerate=samplerate)

    return SoundBuffer(frames, channels=channels, samplerate=samplerate)

cpdef double rand(object lowvalue, object highvalue):
    if isinstance(lowvalue, tuple):
        lowvalue = rand(lowvalue[0], lowvalue[1])

    if isinstance(highvalue, tuple):
        highvalue = rand(highvalue[0], highvalue[1])

    return <double>random.triangular(lowvalue, highvalue)

def find(pattern, channels=2, samplerate=44100):
    """ Glob for files matching a given pattern and return a generator 
        that will `yield` each file as a `SoundBuffer`
    """
    for filename in glob.iglob(pattern, recursive=True):
        yield SoundBuffer(filename, channels=channels, samplerate=samplerate)

def cloud(snd, *args, **kwargs):
    return grains.GrainCloud(snd, *args, **kwargs)

def pool(callback, params, processes=4):
    out = []
    with mp.Pool(processes=processes) as process_pool:
        for result in [ process_pool.apply_async(callback, p) for p in params ]:
            out += [ result.get() ]

    return out

def fill(data, length):
    reps = math.ceil(length / len(data))
    if reps < 2:
        return data

    data = np.hstack([ data for _ in range(reps) ])
    return data[:length]

