import numpy as np
from .soundbuffer import SoundBuffer
from . import wavetables

class Osc:
    """ Wavetable-based oscilator 
    """
    def __init__(self, double freq=440, int offset=0, double amp=1, wavetable=None, double phase=0):
        self.freq = freq
        self.offset = offset
        self.amp = amp
        self.phase = phase

        if wavetable is None:
            wavetable = 'sine'

        if isinstance(wavetable, str):
            self.wavetable = wavetables.wavetable(wavetable, 1024)
        else:
            self.wavetable = wavetable

    def play(self, int length, int channels=2, int samplerate=44100, double amp=1):
        out = np.zeros((length, channels))

        cdef int i = 0
        cdef int wtindex = 0
        cdef int wtlength = len(self.wavetable)
        cdef double val, nextval, frac

        for i in range(length):
            wtindex = int(self.phase) % wtlength

            val = self.wavetable[wtindex]

            try:
                nextval = self.wavetable[wtindex + 1]
            except IndexError:
                nextval = self.wavetable[0]

            frac = self.phase - int(self.phase)

            val = (1.0 - frac) * val + frac * nextval

            for channel in range(channels):
                out[i][channel] = val * amp

            self.phase += self.freq * wtlength * (1.0 / samplerate)

        return SoundBuffer(out, channels=channels, samplerate=samplerate)
