import numpy as np
from .soundbuffer import SoundBuffer
from . import wavetables

class Osc:
    """ Wavetable-based oscilator 
    """
    def __init__(self, freq=440, offset=0, amp=1, wavetable=None):
        self.freq = freq
        self.offset = offset
        self.amp = amp
        self.phase = 0

        if wavetable is None:
            wavetable = 'sine'

        if isinstance(wavetable, str):
            self.wavetable = wavetables.wavetable(wavetable, 1024)
        else:
            self.wavetable = wavetable

    def play(self, length, channels=2, samplerate=44100):
        out = np.zeros((length, channels))

        wtindex = 0
        wtlength = len(self.wavetable)

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
                out[i][channel] = val

            self.phase += self.freq * wtlength * (1.0 / 44100.0)

        return SoundBuffer(out, channels=channels, samplerate=samplerate)
