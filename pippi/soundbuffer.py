import numbers
import math
import random
import reprlib

import numpy as np
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

    def __init__(self, frames=None, length=None, channels=None, samplerate=None):
        self._samplerate = samplerate or DEFAULT_SAMPLERATE
        self._channels = channels or DEFAULT_CHANNELS

        if isinstance(frames, SoundBuffer):
            self.frames = np.ndarray(frames.frames, dtype='d')
            self._channels = frames.channels
            self._samplerate = frames.samplerate
        elif isinstance(frames, str):
            self.frames, self._samplerate = self.read(frames)
            self._channels = self.frames.shape[1]
        elif isinstance(frames, np.ndarray):
            self.frames = np.copy(frames)
            self._channels = self.frames.shape[1]
        elif frames is not None:
            try:
                self.frames = np.fromiter(frames, dtype='d')
                self.frames = np.stack((self.frames for _ in range(self.channels)), 1)
            except TypeError as e:
                raise ValueError('Frames should be an iterable, numpy array, SoundBuffer instance or a filename to read from.') from e
        else:
            self.frames = None

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
            return SoundBuffer(self.frames * value.frames)
        elif isinstance(value, numbers.Real):
            return SoundBuffer(self.frames * float(value))
        else:
            return NotImplemented

    def __imul__(self, value):
        if isinstance(value, SoundBuffer):
            self.frames = self.frames * value.frames
        elif isinstance(value, numbers.Real):
            self.frames = self.frames * float(value)
        else:
            return NotImplemented

        return self

    def __rmul__(self, value):
        return self * value

    def __and__(self, value):
        if isinstance(value, SoundBuffer):
            if len(self.frames) > len(value.frames):
                value.frames.resize(self.frames.shape)
            elif len(value.frames) > len(self.frames):
                self.frames.resize(value.frames.shape)

            return SoundBuffer(self.frames + value.frames)
        else:
            return NotImplemented

    def __iand__(self, value):
        if isinstance(value, SoundBuffer):
            if self.frames is None:
                self.frames = value.frames
            else:
                if len(self.frames) > len(value.frames):
                    value.frames.resize(self.frames.shape)
                elif len(value.frames) > len(self.frames):
                    self.frames.resize(value.frames.shape)

                self.frames = self.frames + value.frames
        else:
            return NotImplemented

        return self

    def __rand__(self, value):
        return self & value

    def __add__(self, value):
        if isinstance(value, SoundBuffer):
            return SoundBuffer(np.concatenate((self.frames, value.frames)))
        elif isinstance(value, numbers.Real):
            return SoundBuffer(self.frames + value)
        else:
            try:
                return SoundBuffer(np.concatenate((self.frames, value)))
            except TypeError:
                return NotImplemented

    def __iadd__(self, value):
        if isinstance(value, SoundBuffer):
            self.frames = np.concatenate((self.frames, value.frames))
        elif isinstance(value, numbers.Real):
            self.frames = self.frames + value
        else:
            try:
                self.frames = np.concatenate((self.frames, value))
            except TypeError:
                return NotImplemented

        return self

    def __radd__(self, value):
        return self + value

    def __sub__(self, value):
        if isinstance(value, SoundBuffer):
            return SoundBuffer(self.frames - value.frames)
        else:
            try:
                return SoundBuffer(self.frames - value)
            except TypeError:
                return NotImplemented

    def __isub__(self, value):
        if isinstance(value, SoundBuffer):
            self.frames = self.frames - value.frames
        else:
            try:
                self.frames = self.frames - value
            except TypeError:
                return NotImplemented

        return self

    def __rsub__(self, value):
        return self - value

    def __bool__(self):
        return bool(len(self))

    def pad(self, start=0, end=0):
        if start > 0:
            silence = np.zeros((start, self.channels))
            self.frames = np.concatenate((silence, self.frames))

        if end > 0:
            silence = np.zeros((end, self.channels))
            self.frames = np.concatenate((self.frames, silence))

    def mix(self, sounds):
        if isinstance(sounds, SoundBuffer):
            self &= sounds
        else:
            try:
                for sound in sounds:
                    self &= sound 
            except TypeError as e:
                raise TypeError('Please provide a SoundBuffer or list of SoundBuffers for mixing') from e

    def dub(self, sounds, pos=0):
        if isinstance(sounds, SoundBuffer):
            if pos > 0:
                sounds.pad(pos) 
            self &= sounds
        else:
            try:
                for sound in sounds:
                    if pos > 0:
                        sound.pad(pos)
                    self &= sound

            except TypeError as e:
                raise TypeError('Please provide a SoundBuffer or list of SoundBuffers for dubbing') from e

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

    def copy(self):
        return SoundBuffer(self.frames, channels=self.channels, samplerate=self.samplerate)

    def pan(self, pos=0.5, method=None):
        """ Pan a stereo sound from pos=0 (hard left) 
            to pos=1 (hard right)

            Different panning strategies can be chosen 
            by passing a value to the `method` param.

            method='constant'
                Constant (square) power panning.
                This is the default.

            method='linear'
                Simple linear panning.

            method='sine'
                Variation on constant power panning 
                using sin() and cos() to shape the pan.
                Taken from the floss manuals csound manual.

            method='gogins'
                Also taken from the csound manual -- 
                Michael Gogins' variation on the above 
                which uses a different part of the sinewave.
        """
        if method is None:
            method = 'constant'

        if method == 'constant':
            self.frames[:,1] *= math.sqrt(pos)
            self.frames[:,0] *= math.sqrt(1 - pos)
        elif method == 'linear':
            self.frames[:,1] *= pos
            self.frames[:,0] *= 1 - pos
        elif method == 'sine':
            self.frames[:,1] *= math.sin(pos * (np.pi / 2))
            self.frames[:,0] *= math.cos(pos * (np.pi / 2))
        elif method == 'gogins':
            self.frames[:,1] *= math.sin((pos + 0.5) * (np.pi / 2))
            self.frames[:,0] *= math.cos((pos + 0.5) * (np.pi / 2))

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

