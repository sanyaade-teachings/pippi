""" Some shortcuts to doing boilerplate work with SoundBuffers
    which make live coding suck less, hopefully.
"""
import glob
import math
import multiprocessing as mp
import numpy as np

from .soundbuffer import SoundBuffer
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

