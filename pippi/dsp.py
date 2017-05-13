""" Some shortcuts to doing boilerplate work with SoundBuffers
    which make live coding suck less, hopefully.
"""
import glob

from .soundbuffer import SoundBuffer

def mix(sounds):
    out = SoundBuffer()
    for sound in sounds:
        out &= sound

    return out

def join(sounds):
    out = SoundBuffer(length=1, channels=2, samplerate=44100)
    for sound in sounds:
        out += sound

    return out

def silence(length=None, channels=None, samplerate=None):
    return SoundBuffer(length=length, channels=channels, samplerate=samplerate)

def read(frames, channels=None, samplerate=None):
    return SoundBuffer(frames, channels=channels, samplerate=samplerate)

def find(pattern, channels=None, samplerate=None):
    for filename in glob.iglob(pattern, recursive=True):
        yield SoundBuffer(filename, channels=channels, samplerate=samplerate)

