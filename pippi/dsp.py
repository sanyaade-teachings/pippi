""" Some shortcuts to doing boilerplate work with SoundBuffers
    which make live coding suck less, hopefully.
"""
import glob

from .soundbuffer import SoundBuffer

def mix(*sounds):
    out = SoundBuffer()
    for sound in tuple(*sounds):
        out &= sound

    return out

def cat(*sounds):
    out = SoundBuffer()
    for sound in tuple(*sounds):
        out += sound

    return out

def silence(length=None, channels=None, samplerate=None):
    return SoundBuffer(length=length, channels=channels, samplerate=samplerate)

def load(frames, channels=None, samplerate=None):
    return SoundBuffer(frames, channels=channels, samplerate=samplerate)

def find(pattern, channels=None, samplerate=None):
    for filename in glob.iglob(pattern, recursive=True):
        yield SoundBuffer(filename, channels=channels, samplerate=samplerate)

