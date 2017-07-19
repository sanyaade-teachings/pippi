# cython: profile=True
import numpy as np
from .soundbuffer import SoundBuffer
from . import wavetables
from . import interpolation

cdef inline MIN_PULSEWIDTH = 0.0001

cdef class Osc:
    """ Wavetable-based oscilator with some extras
    """
    cdef public double freq
    cdef public int offset
    cdef public double amp
    cdef public object wavetable
    cdef public object window
    cdef public object mod
    cdef public double mod_range
    cdef public double mod_freq
    cdef public double pulsewidth
    cdef double phase
    cdef double win_phase
    cdef double mod_phase

    def __init__(
            self, 
            double freq=440, 
            int offset=0, 
            double amp=1, 
            object wavetable=None, 
            object window=None, 
            object mod=None, 
            double mod_range=0.02, 
            double mod_freq=0.1, 
            double phase=0, 
            double pulsewidth=1
        ):

        self.freq = freq
        self.offset = offset
        self.amp = amp
        self.phase = phase
        self.win_phase = phase
        self.mod_phase = phase
        self.mod_range = mod_range 
        self.mod_freq = mod_freq
        self.pulsewidth = pulsewidth if pulsewidth >= MIN_PULSEWIDTH else MIN_PULSEWIDTH

        if wavetable is None:
            wavetable = 'sine'

        if isinstance(wavetable, str):
            self.wavetable = wavetables.wavetable(wavetable, 1024)
        else:
            self.wavetable = wavetable

        if isinstance(window, str):
            self.window = wavetables.window(window, 1024)
        else:
            self.window = window

        if isinstance(mod, str):
            self.mod = wavetables.window(mod, 1024)
        else:
            self.mod = mod

    cpdef object play(self, int length, int channels=2, int samplerate=44100):
        out = np.zeros((length, channels))

        cdef int i = 0
        cdef int wtindex = 0
        cdef int modindex = 0
        cdef int wtlength = len(self.wavetable)
        cdef int modlength = 1 if self.mod is None else len(self.mod)
        cdef double val, val_mod, nextval, nextval_mod, frac, frac_mod

        wavetable = self.wavetable
        if self.window is not None:
            window = interpolation.linear(self.window, wtlength)
            wavetable = np.multiply(wavetable, window)

        if self.pulsewidth < 1:
            silence_length = int((wtlength / self.pulsewidth) - wtlength)
            wavetable = np.concatenate((wavetable, np.zeros(silence_length)))

        for i in range(length):
            wtindex = int(self.phase) % wtlength
            modindex = int(self.mod_phase) % modlength

            val = wavetable[wtindex]
            val_mod = 1

            try:
                nextval = wavetable[wtindex + 1]
            except IndexError:
                nextval = wavetable[0]

            if self.mod is not None:
                try:
                    nextval_mod = self.mod[modindex + 1]
                except IndexError:
                    nextval_mod = self.mod[0]

                frac_mod = self.mod_phase - int(self.mod_phase)
                val_mod = self.mod[modindex]
                val_mod = (1.0 - frac_mod) * val_mod + frac_mod * nextval_mod
                val_mod = 1.0 + (val_mod * self.mod_range)
                self.mod_phase += self.mod_freq * modlength * (1.0 / samplerate)

            frac = self.phase - int(self.phase)
            val = (1.0 - frac) * val + frac * nextval
            self.phase += self.freq * val_mod * wtlength * (1.0 / samplerate)

            for channel in range(channels):
                out[i][channel] = val * self.amp

        return SoundBuffer(out, channels=channels, samplerate=samplerate)


