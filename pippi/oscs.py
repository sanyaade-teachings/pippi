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
        out, self.phase = wavetables.interp(self.wavetable, length, self.freq, self.phase, channels, samplerate)

        return SoundBuffer(out, channels=channels, samplerate=samplerate)
