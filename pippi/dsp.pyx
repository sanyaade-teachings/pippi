import glob
import math
import multiprocessing as mp
import numpy as np
import random

cimport cython

from pippi.soundbuffer import SoundBuffer
from pippi.soundbuffer cimport SoundBuffer
from pippi cimport wavetables as wts
from pippi cimport rand as _rand
from pippi import drummachine as dm

# Expose some C flags / constants to python
# FIXME might be faster to use newish cpdef enum defs? donno
SINE = wts.SINE
SINEIN = wts.SINEIN
SINEOUT = wts.SINEOUT
COS = wts.COS
TRI = wts.TRI
SAW = wts.SAW
PHASOR = wts.PHASOR
LINE = wts.LINE
RSAW = wts.RSAW
HANN = wts.HANN
HANNIN = wts.HANNIN
HANNOUT = wts.HANNOUT
HAMM = wts.HAMM
BLACK = wts.BLACK
BLACKMAN = wts.BLACK
BART = wts.BART
BARTLETT = wts.BARTLETT
KAISER = wts.KAISER
SQUARE = wts.SQUARE
RND = wts.RND
LINEAR = wts.LINEAR
TRUNC = wts.TRUNC
HERMITE = wts.HERMITE
CONSTANT = wts.CONSTANT
GOGINS = wts.GOGINS

# Just a shorthand for MS in scripts. 
# For example:
#
# >>> snd.cut(0.75, 32*dsp.MS)
#
# Which cuts, starting from 0.75 seconds in, 
# a 32 millisecond section from the SoundBuffer `snd`.
MS = 0.001

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double _mag(double[:,:] snd):
    cdef int i = 0
    cdef int c = 0
    cdef int framelength = len(snd)
    cdef int channels = snd.shape[1]
    cdef double maxval = 0

    for i in range(framelength):
        for c in range(channels):
            maxval = max(maxval, abs(snd[i,c]))

    return maxval

cpdef double mag(SoundBuffer snd):
    return _mag(snd.frames)


cpdef SoundBuffer mix(list sounds, align_end=False):
    """ Mix a list of sounds into a new sound
    """
    cdef double maxlength = 0
    cdef int maxchannels = 1
    cdef SoundBuffer sound

    for sound in sounds:
        maxlength = max(maxlength, sound.dur)
        maxchannels = max(maxchannels, sound.channels)

    cdef SoundBuffer out = SoundBuffer(length=maxlength, channels=maxchannels)

    if align_end:
        for sound in sounds:
            out.dub(sound, maxlength - sound.dur)
    else:
        for sound in sounds:
            out.dub(sound, 0)

    return out
    
cpdef wts.Wavetable randline(int numpoints, double lowvalue=0, double highvalue=1, int wtsize=4096):
    return wts.randline(numpoints, lowvalue, highvalue, wtsize)

cpdef wts.Wavetable wt(object values, 
        object lowvalue=None, 
        object highvalue=None, 
        object wtsize=None, 
        bint window=False
    ):
    return wts.Wavetable(values, lowvalue, highvalue, wtsize, window)

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

    # TODO
    while copied_channels < channels:
        out.frames.base[:,channel_to_copy] = sounds[current_sound_index].frames.base[:,source_channel_to_copy]    
        copied_channels += 1

    return out

def join(sounds, overlap=None, channels=None, samplerate=None):
    """ Concatenate a list of sounds into a new sound
    """
    channels = channels or sounds[0].channels
    samplerate = samplerate or sounds[0].samplerate

    cdef double total_length = 0
    cdef SoundBuffer sound

    for sound in sounds:
        total_length += sound.dur
        if overlap is not None:
            total_length -= overlap

    out = SoundBuffer(length=total_length, channels=channels, samplerate=samplerate)

    pos = 0
    for sound in sounds:
        out.dub(sound, pos)
        pos += sound.dur
        if overlap is not None:
            pos -= overlap

    return out

cpdef SoundBuffer buffer(object frames=None, double length=-1, int channels=2, int samplerate=44100):
    return SoundBuffer.__new__(SoundBuffer, frames=frames, length=length, channels=channels, samplerate=samplerate)

cpdef SoundBuffer read(object filename, double length=-1, double start=0):
    """ Read a soundfile from disk and return a `SoundBuffer` with its contents.
        May include a start position and length in seconds to read a segment from a large file.

        The `filename` param is always converted to a string, so it is safe to pass a 
        `Path` instance from the standard library `pathlib` module.
    """
    return SoundBuffer(filename=str(filename), length=length, start=start)

cpdef double rand(double low=0, double high=1):
    return _rand.rand(low, high)

cpdef int randint(int low=0, int high=1):
    return _rand.randint(low, high)

cpdef object choice(list choices):
    return _rand.choice(choices)

def redrum(*args, **kwargs):
    return dm.DrumMachine(*args, **kwargs)

def find(pattern, channels=2, samplerate=44100):
    """ Glob for files matching a given pattern and return a generator 
        that will `yield` each file as a `SoundBuffer`
    """
    for filename in glob.iglob(pattern, recursive=True):
        yield SoundBuffer(channels=channels, samplerate=samplerate, filename=filename)

def pool(callback, reps=None, params=None, processes=None):
    out = []
    if params is None:
        params = [(None,)]

    if reps is None:
        reps = len(params)

    if processes is None:
        processes = mp.cpu_count()

    with mp.Pool(processes=processes) as process_pool:
        for result in [ process_pool.apply_async(callback, params[i % len(params)]) for i in range(reps) ]:
            out += [ result.get() ]

    return out

def fill(data, length):
    reps = math.ceil(length / len(data))
    if reps < 2:
        return data

    data = np.hstack([ data for _ in range(reps) ])
    return data[:length]

