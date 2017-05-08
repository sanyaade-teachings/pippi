from array import array
import numpy as np
import random
import reprlib
import soundfile

from . import wavetables

DEFAULT_SAMPLERATE = 44100
DEFAULT_CHANNELS = 2
DEFAULT_SOUNDFILE = 'wav'

class SoundBuffer:
    """ A sequence of audio frames 
        representing a buffer of sound.
    """

    @property
    def samplerate(self):
        """ TODO The samplerate of the buffer will be used when 
            combining buffers (mixing, concatenating, etc) 
            in a way that produces a new buffer. The resulting 
            buffer will be at the higher of the two rates, 
            with the lower up-sampled if needed.

            Or just provide conversation functions & require that 
            sr between samples matches?
        """
        return self._samplerate

    @property
    def channels(self):
        """ TODO also work out mixing rules when combining buffers 
            with different numbers of channels.
        """
        return self._channels


    def __init__(self, filename=None, length=None, channels=None, samplerate=None, frames=None):
        self._samplerate = samplerate or DEFAULT_SAMPLERATE
        self._channels = channels or DEFAULT_CHANNELS
        self.frames = frames

        if filename is not None:
            self.frames, self._samplerate = self.read(filename)

        if length is not None:
            if self.frames is not None:
                self.fill(length)
            else:
                self.clear(length)

    def __len__(self):
        return 0 if self.frames is None else len(self.frames)

    def __getitem__(self, position):
        if isinstance(position, int):
            return tuple(self.frames[position])

        return SoundBuffer(frames=self.frames[position])

    def __repr__(self):
        return 'SoundBuffer(samplerate={}, channels={}, frames={})'.format(self.samplerate, self.channels, self.frames)

    def __iter__(self):
        return self.grains(1)

    def __mul__(self, value):
        if isinstance(value, SoundBuffer):
            return SoundBuffer(frames=self.frames * value.frames)
        return SoundBuffer(frames=np.tile(self.frames, (int(value), 1)))

    def __rmul__(self, value):
        return self * value

    def __add__(self, value):
        if len(self) == 0:
            return value
        elif len(value) == 0:
            return self

        if isinstance(value, SoundBuffer):
            return SoundBuffer(frames=np.concatenate((self.frames, value.frames)))
        elif isinstance(value, int):
            # What do we do here?
            pass

    def __radd__(self, value):
        return self + value

    def __bool__(self):
        return bool(len(self))

    def clear(self, length=None):
        """ Replace the buffer with a new empty buffer
            of the given length in frames.
        """
        if length is None:
            self.frames = None
        else:
            self.frames = np.zeros((length, self.channels))

    def write(self, filename=None, timestamp=False):
        """ Write the contents of this buffer to disk 
            in the given audio file format. (WAV, AIFF, AU)
        """
        return soundfile.write(filename, self.frames, self.samplerate)

    def read(self, filename):
        """ Read the contents of a sound file into 
            the buffer as frames.
        """
        return soundfile.read(filename)

    def grains(self, minlength, maxlength=None):
        """ Iterate over the buffer in fixed-size grains.
            If a second length is given, iterate in randomly-sized 
            grains, given the minimum and maximum sizes.
        """
        framesread = 0
        grainlength = minlength
        while framesread < len(self):
            if maxlength is not None:
                grainlength = random.randint(minlength, maxlength)

            try:
                yield self[framesread:framesread+grainlength]
            except IndexError:
                yield self[framesread:]

            framesread += grainlength

    def env(self, window_type=None, values=None, amp=1.0):
        """ Apply an amplitude envelope or 
            window to the sound of the given envelope 
            type -- or if a list of `values` is provided, 
            use it as an interpolated amplitude wavetable.
        """
        wavetable = wavetables.window(
                        window_type=window_type, 
                        values=values, 
                        length=len(self)
                    )
        wavetable = wavetable.reshape((len(self), 1))
        wavetable = np.repeat(wavetable, self.channels, axis=1)

        return SoundBuffer(frames=self.frames * wavetable * amp)

    def fill(self, length):
        """ Truncate the buffer to the given length or 
            loop the contents of the buffer up to the 
            given length in frames.
        """
        mult = 0 if length <= 0 or len(self) == 0 else length / len(self)

        if mult < 1:
            self.frames = self.frames[:length]
        elif mult > 1:
            if int(mult) > 1:
                self.frames = np.tile(self.frames, (int(mult), 1))
            self.frames = np.concatenate((self.frames, self.frames[:length - len(self.frames)]))
        elif mult <= 0:
            self.clear()


    def speed(self, speed):
        """ TODO Change the pitch and the length of the sound
        """
        pass

    def transpose(self, factor):
        """ TODO Change the pitch of the sound without changing 
            the length.
            Should accept: from/to hz, notes, midi notes, intervals
        """
        pass

    def stretch(self, length):
        """ TODO Change the length of the sound without changing 
            the pitch.
        """
        pass

