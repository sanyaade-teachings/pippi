""" Some shortcuts to doing boilerplate work with SoundBuffers
    which make live coding suck less, hopefully.
"""
import glob
import math
import multiprocessing as mp
import numpy as np

from .soundbuffer import SoundBuffer

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

