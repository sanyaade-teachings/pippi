#cython: language_level=3

from pippi.soundbuffer import SoundBuffer
from pippi import tune
import random

cdef class Sampler:
    def __init__(Sampler self, dict sounds):
        self.snds = []
        self.sndmap = {}

        cdef double lastroot = 0
        cdef str note
        cdef list snds

        for note, snds in sounds.items():
            root = tune.ntf(note)
            self.snds += [(root, snds)]

    cpdef SoundBuffer play(Sampler self, double freq=440, object length=None):
        cdef double root
        cdef SoundBuffer snd
        cdef double speed = 1

        root, snd = self.getsnd(freq)
        speed = freq / root

        if length is None:
            snd = snd.speed(speed)
        else:
            snd = snd.transpose(speed, length)

        return snd

    cpdef tuple getsnd(Sampler self, double freq):
        cdef double root = 0
        cdef int i = 0
        cdef list snds = []

        if freq in self.sndmap:
            return (self.snds[self.sndmap[freq]][0], random.choice(self.snds[self.sndmap[freq]][1]).copy())

        for i, (root, snds) in enumerate(self.snds):
            if freq > root:
                self.sndmap[freq] = i
                return (root, random.choice(snds).copy())

        return (self.snds[0][0], random.choice(self.snds[0][1]).copy())

