import numbers
import math
import random
import reprlib

import soundfile
import numpy as np

from . import wavetables
from . import interpolation

cdef inline int DEFAULT_SAMPLERATE = 44100
cdef inline int DEFAULT_CHANNELS = 2
cdef inline unicode DEFAULT_SOUNDFILE = u'wav'

cdef class SoundBuffer:
    """ A sequence of audio frames 
        representing a buffer of sound.
    """

    cdef public int samplerate
    cdef public int channels
    cdef public double[:,:] frames

    def __cinit__(self, double[:,:] frames=None, int length=-1, int channels=DEFAULT_CHANNELS, int samplerate=DEFAULT_SAMPLERATE, unicode filename=None, int start=-1):
        """ Only ever run once -- python __init__ may run more than once or not at all.
            Creates initial buffer from inputs and sets channels, samplerate
        """
        self.samplerate = samplerate
        self.channels = channels

        if filename is not None:
            #self.frames, self.samplerate = soundfile.read(filename, frames=length, start=start, dtype='float64')
            self.frames, self.samplerate = soundfile.read(filename, dtype='float64')
            self.channels = self.frames.shape[1]
        elif frames is not None:
            self.frames = frames
            self.channels = frames.shape[1]
        elif length > 0:
            self.frames = np.zeros((length, self.channels))
        else:
            self.frames = None

    def _fill(self, double[:,:] frames):
        """ Fill current w/frames
        """
        cdef int frame_index, sample_index = 0
        for frame_index, frame in enumerate(frames):
            for sample_index, sample in enumerate(frame):
                self.frames[frame_index][sample_index] = sample

    def copy(self):
        if self.frames is None:
            return SoundBuffer(channels=self.channels, samplerate=self.samplerate)
        cdef double[:, :] out = np.zeros((len(self), self.channels), dtype='d')
        out[:,:] = self.frames
        return SoundBuffer(out, self.channels, self.samplerate)

    def __len__(self):
        """ Return the length of this SoundBuffer in frames

            >>> sound = SoundBuffer(length=44100)
            >>> len(sound)
            44100
        """
        return 0 if self.frames is None else len(self.frames)

    def __getitem__(self, position):
        """ Slice into this SoundBuffer, returning a copy of a portion of the buffer, 
            or a tuple of samples representing a single frame.

            >>> sound = SoundBuffer(length=44100, channels=2)
            >>> bit = sound[0:100]
            >>> len(bit)
            100
            >>> sound[0]
            (0,0)
        """
        if isinstance(position, int):
            return tuple(self.frames[position])

        cdef double[:, :] out

        if not position.stop:
            out = self.frames[position.start:]

        elif not position.start:
            out = self.frames[:position.stop]

        else:
            out = self.frames[position.start:position.stop]

        return SoundBuffer(out, self.channels, self.samplerate)

    def __repr__(self):
        """ Return a string representation of this SoundBuffer
        """
        return 'SoundBuffer(samplerate={}, channels={}, frames={})'.format(self.samplerate, self.channels, self.frames)

    def __mul__(self, value):
        """ Multiply a SoundBuffer by a number, iterable or SoundBuffer and return a new SoundBuffer

            >>> sound = SoundBuffer(length=44100, channels=2, samplerate=44100)

            # Multiply every sample in `sound` by 0.5
            >>> sound *= 0.5

            # Multiply every sample by an interpolated copy of `wavetable` 
            # `wavetable` can be a 1d flat array/list/iterable, or a SoundBuffer
            >>> wavetable = [0, 0.25, 0.5, 0.25, 0.75, 1]
            >>> sound = sound * wavetable

            >>> sound2 = SoundBuffer(length=2000, channels=2, samplerate=44100)
            >>> sound = sound * sound2

        """
        cdef int length = len(self)
        cdef double[:, :] out
        cdef double[:] wavetable

        if isinstance(value, numbers.Real):
            out = np.multiply(self.frames, value)
            return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

        elif isinstance(value, SoundBuffer):
            value = value.speed(length // len(value))
            out = np.multiply(self.frames, value.frames)
            return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

        else:
            try:
                wavetable = interpolation.linear(value, length)
                out = np.multiply(self.frames, wavetable[:,None])
                return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)
            except TypeError as e:
                raise NotImplemented('Cannot multiply SoundBuffer with %s' % type(value)) from e

    def __imul__(self, value):
        """ Multiply a SoundBuffer in place by a number, iterable or SoundBuffer
        """
        cdef int length = len(self)
        if isinstance(value, numbers.Real):
            self.frames = np.multiply(self.frames, value)

        elif isinstance(value, SoundBuffer):
            value = value.speed(length // len(value))
            self.frames = np.multiply(self.frames, value.frames)

        else:
            try:
                value = interpolation.linear(value, length)
                self.frames = np.multiply(self.frames, value[:,None])
            except TypeError as e:
                raise NotImplemented('Cannot multiply SoundBuffer with %s' % type(value)) from e

        return self

    def __rmul__(self, value):
        return self * value

    def __and__(self, value):
        """ Mix two SoundBuffers or two 2d arrays with compatible dimensions

            >>> sound = SoundBuffer(length=44100, channels=2, samplerate=44100)
            >>> sound2 = SoundBuffer(length=2000, channels=2, samplerate=44100)
            >>> out = sound2 & sound
            >>> len(out)
            44100
        """

        cdef double[:,:] out

        try:
            out = np.add(self.frames, value[:len(self.frames)])
            return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)
        except TypeError as e:
            return NotImplemented

    def __iand__(self, value):
        """ Mix in place two SoundBuffers or two 2d arrays with compatible dimensions
        """
        self.frames = np.add(self.frames, value[:len(self.frames)])
        return self

    def __rand__(self, value):
        return self & value

    def __add__(self, value):
        """ Add a number to every sample in this SoundBuffer or concatenate with another 
            SoundBuffer or iterable with compatible dimensions
        """
        if self.frames is None:
            if isinstance(value, numbers.Real):
                return SoundBuffer(channels=self.channels, samplerate=self.samplerate)
            elif isinstance(value, SoundBuffer):
                return value.copy()
            else:
                return SoundBuffer(value, channels=self.channels, samplerate=self.samplerate)

        cdef int length = len(self)
        cdef double[:,:] out = np.zeros((length, self.channels))

        if isinstance(value, numbers.Real):
            out = np.add(self.frames, value)
        elif isinstance(value, SoundBuffer):
            if value.frames is None:
                out = self.frames.copy()
            else:
                out = np.vstack((self.frames, value.frames))
        else:
            try:
                out = np.vstack((self.frames, value))
            except TypeError as e:
                return NotImplemented

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

    def __iadd__(self, value):
        """ In place add either adding number to every value without copy, or 
            directly extending internal frame buffer.
        """
        if self.frames is None:
            if isinstance(value, numbers.Real):
                return self
            elif isinstance(value, SoundBuffer):
                return value.copy()
            else:
                return SoundBuffer(value, channels=self.channels, samplerate=self.samplerate)

        if isinstance(value, numbers.Real):
            self.frames = np.add(self.frames, value)
        else:
            try:
                self.frames = np.vstack((self.frames, value))
            except TypeError as e:
                return NotImplemented

        return self

    def __radd__(self, value):
        return self + value

    def __sub__(self, value):
        """ Subtract a number from every sample in this SoundBuffer or  
            from another SoundBuffer or iterable with compatible dimensions
        """
        cdef double[:,:] out

        if isinstance(value, numbers.Real):
            out = np.subtract(self.frames, value)
        if isinstance(value, SoundBuffer):
            out = np.subtract(self.frames, value.frames)
        else:
            try:
                out = np.subtract(self.frames, value[:,None])
            except TypeError as e:
                return NotImplemented

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

    def __isub__(self, value):
        """ In place subtract a number from every sample in this SoundBuffer or  
            from another SoundBuffer or iterable with compatible dimensions
        """
        if isinstance(value, numbers.Real):
            self.frames = np.subtract(self.frames, value)
        if isinstance(value, SoundBuffer):
            self.frames = np.subtract(self.frames, value.frames)
        else:
            try:
                self.frames = np.subtract(self.frames, value.frames)
            except TypeError as e:
                return NotImplemented

    def __rsub__(self, value):
        return self - value

    def __bool__(self):
        return bool(len(self))

    def pad(self, int start=0, int end=0):
        """ Pad this sound in place with silence at start or end
        """
        if start <= 0 and end <= 0: 
            return self

        cdef double[:,:] out

        if start > 0:
            out = np.vstack((np.zeros((start, self.channels)), self.frames))

        if end > 0:
            out = np.vstack((self.frames, np.zeros((end, self.channels))))
 
        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

    def mix(self, sounds):
        """ Mix this sound in place with a sound or iterable of sounds
        """
        if isinstance(sounds, SoundBuffer):
            self &= sounds
        else:
            try:
                for sound in sounds:
                    self &= sound 
            except TypeError as e:
                raise TypeError('Please provide a SoundBuffer or list of SoundBuffers for mixing') from e

    def _dub(self, SoundBuffer sound, int pos):
        cdef int dublength = len(sound) + pos

        if len(self) < dublength:
            if self.frames is not None:
                self.frames = np.vstack((self.frames, np.zeros((dublength, self.channels))))
            else:
                self.frames = np.zeros((dublength, self.channels))

        self.frames.base[pos:dublength] += sound.frames

    def dub(self, sounds, int pos=0):
        """ Dub a sound or iterable of sounds into this soundbuffer
        """
        if isinstance(sounds, SoundBuffer):
            self._dub(sounds, pos)
        else:
            try:
                for sound in sounds:
                    self._dub(sound, pos)

            except TypeError as e:
                raise TypeError('Please provide a SoundBuffer or list of SoundBuffers for dubbing') from e

    def repeat(self, int reps=2):
        if reps <= 1:
            return SoundBuffer(self.frames)

        out = SoundBuffer(self.frames)
        for _ in range(reps-1):
            out += self

        return out

    def cut(self, int start=0, int length=44100):
        return SoundBuffer(self.frames[start:start+length])

    def rcut(self, int length=44100):
        cdef int maxlen = len(self) - length
        if maxlen <= 0:
            return self
        cdef int start = random.randint(0, maxlen)
        return self.cut(start, length)

    def clear(self, int length=-1):
        """ Writes zeros into the buffer, adjusting the
            size if length > 0.
        """
        if length <= 0:
            self.frames = None
        else:
            self.frames = np.zeros((length, self.channels))

    def write(self, unicode filename=None):
        """ Write the contents of this buffer to disk 
            in the given audio file format. (WAV, AIFF, AU)
        """
        return soundfile.write(filename, self.frames, self.samplerate)

    def grains(self, int minlength, int maxlength=-1):
        """ Iterate over the buffer in fixed-size grains.
            If a second length is given, iterate in randomly-sized 
            grains, given the minimum and maximum sizes.
        """
        cdef int framesread = 0
        cdef int grainlength = minlength
        while framesread < len(self):
            if maxlength > 0:
                grainlength = random.randint(minlength, maxlength)

            try:
                yield self[framesread:framesread+grainlength]
            except IndexError:
                yield self[framesread:]

            framesread += grainlength

    def pan(self, double pos=0.5, unicode method=None):
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
            method = u'constant'

        cdef double[:,:] out = self.frames
        cdef int length = len(self)

        if method == 'constant':
            out.base[:,0] *= math.sqrt(pos)
            out.base[:,1] *= math.sqrt(1 - pos)
        elif method == 'linear':
            out.base[:,0] *= pos
            out.base[:,1] *= 1 - pos
        elif method == 'sine':
            out.base[:,0] *= math.sin(pos * (math.pi / 2))
            out.base[:,1] *= math.cos(pos * (math.pi / 2))
        elif method == 'gogins':
            out.base[:,0] *= math.sin((pos + 0.5) * (math.pi / 2))
            out.base[:,1] *= math.cos((pos + 0.5) * (math.pi / 2))

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

    def env(self, unicode window_type=None, list values=None):
        """ Apply an amplitude envelope or 
            window to the sound of the given envelope 
            type -- or if a list of `values` is provided, 
            use it as an interpolated amplitude wavetable.
        """
        cdef int length = len(self)
        cdef double[:] wavetable = None
        if values is not None:
            wavetable = interpolation.linear(values, length)
        else:
            wavetable = wavetables.window(
                            window_type=window_type, 
                            length=length,
                        )

        if wavetable is None:
            raise TypeError('Invalid envelope type')

        return self * wavetable

    def taper(self, int length):
        out = SoundBuffer(frames=self.frames[:,:], channels=self.channels, samplerate=self.samplerate)
        out[:length] *= wavetables.window('line', length)
        out[:-length] *= wavetables.window('phasor', length)
        return out

    def fill(self, int length):
        """ Truncate the buffer to the given length or 
            loop the contents of the buffer up to the 
            given length in frames.
        """
        cdef int mult = 0 if length <= 0 or len(self) == 0 else length // len(self)
        out = SoundBuffer(frames=self.frames[:,:], channels=self.channels, samplerate=self.samplerate)

        if mult > 1:
            out = out.repeat(mult)

        return out[:length]

    def speed(self, double speed):
        """ Change the speed of the sound
        """
        speed = speed if speed > 0 else 0.001
        cdef int length = int(len(self) * (1.0 / speed))
        out_o = np.zeros((length, self.channels))

        cdef int channel
        for channel in range(self.channels):
            out_o[:,channel] = interpolation.linear(np.asarray(self.frames[:,channel]), length)

        cdef double[:,:] out = out_o

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

    def transpose(self, factor):
        """ TODO Change the pitch of the sound without changing 
            the length.
            Should accept: from/to hz, notes, midi notes, intervals
        """
        return NotImplemented

    def stretch(self, int length):
        """ TODO Change the length of the sound without changing 
            the pitch.
        """
        return NotImplemented
