import numbers
import random
import reprlib

import soundfile
import numpy as np
cimport cython
from libc cimport math

from . cimport wavetables
from . cimport interpolation
from . cimport grains
from .defaults cimport DEFAULT_SAMPLERATE, DEFAULT_CHANNELS, DEFAULT_SOUNDFILE

cdef double[:,:] _remix(double[:,:] values, int framelength, int channels):
    if len(values) == framelength and values.shape[1] == channels:
        return values

    cdef double[:,:] out = None
    cdef int i = 0
    cdef int channel = 0

    if values.shape[1] == channels:
        for channel in range(channels):
            out[:,channel] = interpolation._linear(values[:,channel], framelength)
        return out

    elif channels <= 1:
        return np.sum(values[i], 1)

    else:
        out = np.zeros((framelength, channels))
        for i in range(framelength):
            out[i] = interpolation._linear(values[i], channels)
        return out

cdef double[:,:] _mul2d(double[:,:] output, double[:,:] values):
    return np.multiply(output, _remix(values, len(output), output.shape[1]))

cdef double[:,:] _mul1d(double[:,:] output, double[:] values):
    cdef int channel = 0
    cdef int channels = output.shape[1]
    cdef int framelength = len(output)

    if len(values) != framelength:
        values = interpolation._linear(values, framelength)

    for channel in range(channels):
        output.base[:,channel] *= values

    return output

cdef class SoundBuffer:
    """ A sequence of audio frames 
        representing a buffer of sound.
    """

    def __cinit__(self, 
            object frames=None, 
            double length=-1, 
               int channels=-1, 
               int samplerate=DEFAULT_SAMPLERATE, 
           unicode filename=None, 
            double start=0, 
       double[:,:] buf=None):
        """ Data may be loaded into the SoundBuffer from these sources as keyword arguments, 
            in decending order of precedence:

              - `filename`: a unicode string relative path or a full path to load from a soundfile
              - `buf`: a memoryview compatible 2D buffer of doubles <double[:,:]>
              - `frames`: a 1D or 2D python iterable (eg a list) or a 1D numpy array/buffer

            Data loaded via the `filename` keyword can load any soundfile type supported by libsndfile,
            including old favorites like WAV, AIFF, FLAC, and OGG. But no, not MP3. (At least not yet: https://github.com/erikd/libsndfile/issues/258)
            If the `length` param is given, only that amount will be read from the soundfile.
            Use the `start` param to specify a position (seconds) in the soundfile to begin reading from.
            No samplerate conversion is done (yet) so be careful to match the samplerate of the start param with that 
            of the soundfile being read. (Or don't!)
            Overflows (reading beyond the boundries of the file) are filled with silence so the length 
            param is always respected and will return a SoundBuffer of the requested length. This is 
            just a wrapper around pysndfile's soundfile.read method.

            Data loaded via the `buf` keyword must be interpretable by cython as a memoryview on a 2D array 
            of doubles. (A numpy ndarray for example.)

            Single channel (1D) data loaded via the `frames` keyword will be copied into the given number of channels
            specified by the `channels` param. (And if no value is given, a mono SoundBuffer will be created.) 
            Handy for doing synthesis prototyping with python lists.

            Multichannel (2D) data loaded via the `frames` keyword will ignore the `channels` 
            param and adapt to what was given by attempting to read the data into a numpy array
            and take the number of channels from its shape.
            
            The remaining sources (`filename`, `buf`) will use the number of channels found in the 
            data and ignore the value passed to the SoundBuffer initializer.

            If none of those sources are present, but a `length` param has been given, 
            the SoundBuffer will be initialized with silence to the given length in seconds.
            
            Otherwise an empty SoundBuffer is created with a length of zero.
        """
        self.samplerate = samplerate
        self.channels = channels
        cdef int framestart = 0
        cdef int framelength = <int>(length * self.samplerate)

        if filename is not None:
            framestart = <int>(start * self.samplerate)
            self.frames, self.samplerate = soundfile.read(filename, framelength, framestart, dtype='float64', fill_value=0)
            self.channels = self.frames.shape[1]

        elif buf is not None:
            self.frames = buf
            self.channels = self.frames.shape[1]

        elif frames is not None:
            if isinstance(frames, list) or len(frames.shape) == 1:
                if self.channels <= 0:
                    self.channels = 1
                self.frames = np.column_stack([ frames for _ in range(self.channels) ])
            else:
                self.frames = np.array(frames, dtype='d')
                self.channels = self.frames.shape[1]

        elif length > 0:
            if self.channels <= 0:
                self.channels = DEFAULT_CHANNELS
            self.frames = np.zeros((framelength, self.channels))
        else:
            self.frames = None


    #############################################
    # (+) Addition & concatenation operator (+) #
    #############################################
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

        cdef int length = len(self.frames)
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


    cdef SoundBuffer _adsr(self, double attack, double decay, double sustain, double release):
        cdef int framelength = len(self)
        cdef int attack_breakpoint = <int>(<double>self.samplerate * attack)
        cdef int decay_breakpoint = <int>(<double>self.samplerate * decay) + attack_breakpoint
        cdef int sustain_breakpoint = framelength - <int>(release * <double>self.samplerate)
        cdef int decay_framelength = decay_breakpoint - attack_breakpoint
        cdef int release_framelength = framelength - sustain_breakpoint

        for i in range(framelength):
            if i <= attack_breakpoint and attack_breakpoint > 0:
                self.frames.base[i] = self.frames.base[i] * (i / <double>attack_breakpoint)

            elif i <= decay_breakpoint and decay_breakpoint > 0:
                self.frames.base[i] = self.frames.base[i] * ((1 - ((i - attack_breakpoint) / <double>decay_framelength)) * (1 - sustain) + sustain)
        
            elif i <= sustain_breakpoint:
                self.frames.base[i] = self.frames.base[i] * sustain

            else:
                self.frames.base[i] = self.frames.base[i] * ((1 - ((i - sustain_breakpoint) / <double>release_framelength)) * sustain)

        return self

    def adsr(self, double a=0, double d=0, double s=1, double r=0):
        return self._adsr(a, d, s, r)


    ########################
    # (&) Mix operator (&) #
    ########################
    def __and__(self, value):
        """ Mix two SoundBuffers or two 2d arrays with compatible dimensions

            >>> sound = SoundBuffer(length=1, channels=2, samplerate=44100)
            >>> sound2 = SoundBuffer(length=0.35, channels=2, samplerate=44100)
            >>> out = sound2 & sound
            >>> len(out)
            1.0
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


    ##############
    # Truthiness #
    ##############
    def __bool__(self):
        return bool(len(self))


    #############################
    # ([:]) Frame slicing ([:]) #
    #############################
    def __getitem__(self, position):
        """ Slice into this SoundBuffer, returning a copy of a portion of the buffer, 
            or a tuple of samples representing a single frame.

            >>> sound = SoundBuffer(length=1, samplerate=44100, channels=2)
            >>> bit = sound[0:4410]
            >>> len(bit)
            0.1
            >>> sound[0]
            (0,0)
        """
        if isinstance(position, int):
            return tuple(self.frames[<int>position])

        cdef double[:, :] out

        if not position.stop:
            out = self.frames[position.start:]

        elif not position.start:
            out = self.frames[:position.stop]

        else:
            out = self.frames[position.start:position.stop]

        return SoundBuffer(out, self.channels, self.samplerate)


    ###############
    # Lengthiness #
    ###############
    def __len__(self):
        """ Return the length of this SoundBuffer in frames.
            >>> sound = SoundBuffer(length=1, samplerate=44100, channels=2)
            >>> len(sound)
            44100
        """
        return 0 if self.frames is None else len(self.frames)

    @property
    def dur(self):
        """ Duration of the SoundBuffer in seconds
        """
        return len(self) / <double>self.samplerate


    ###################################
    # (*) Multiplication operator (*) #
    ###################################
    def __mul__(self, value):
        """ Multiply a SoundBuffer by a number, iterable or SoundBuffer and return a new SoundBuffer

            >>> sound = SoundBuffer(length=1, channels=2, samplerate=44100)

            # Multiply every sample in `sound` by 0.5
            >>> sound *= 0.5

            # Multiply every sample by an interpolated copy of `wavetable` 
            # `wavetable` can be a 1d flat array/list/iterable, or a SoundBuffer
            # This will modulate the amplitude of the SoundBuffer with the wavetable.
            >>> wavetable = [0, 0.25, 0.5, 0.25, 0.75, 1]
            >>> sound = sound * wavetable

            >>> sound2 = SoundBuffer(length=0.5, channels=2, samplerate=44100)
            >>> sound = sound * sound2

        """
        cdef int length = len(self.frames)
        cdef double[:,:] out
        cdef double[:] wavetable

        if isinstance(value, numbers.Real):
            out = np.multiply(self.frames, value)

        elif isinstance(value, SoundBuffer):
            out = _mul2d(self.frames, value.frames)

        elif isinstance(value, list):
            out = _mul1d(self.frames, np.array(value))

        else:
            try:
                if len(value.shape) == 1:
                    out = _mul1d(self.frames, value)
                else:
                    out = _mul2d(self.frames, value)
            except TypeError:
                return NotImplemented

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

    def __imul__(self, value):
        """ Multiply a SoundBuffer in place by a number, iterable or SoundBuffer
        """
        if isinstance(value, numbers.Real):
            self.frames = np.multiply(self.frames, value)

        elif isinstance(value, SoundBuffer):
            self.frames = _mul2d(self.frames, value.frames)

        elif isinstance(value, list):
            self.frames = _mul1d(self.frames, np.array(value))

        else:
            try:
                if len(value.shape) == 1:
                    self.frames = _mul1d(self.frames, value)
                else:
                    self.frames = _mul2d(self.frames, value)
            except TypeError:
                return NotImplemented

        return self

    def __rmul__(self, value):
        return self * value


    ################################
    # (-) Subtraction operator (-) #
    ################################
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


    ##############################
    # Pickling & stringification #
    ##############################
    def __reduce__(self):
        """ Tells the pickle module how to rebuild a pickled instance of a SoundBuffer
        """
        return (rebuild_buffer, (np.asarray(self.frames), self.channels, self.samplerate))

    def __repr__(self):
        """ Return a string representation of this SoundBuffer
        """
        return 'SoundBuffer(samplerate={}, channels={}, frames={})'.format(self.samplerate, self.channels, self.frames)


    ##########################
    # Transformation methods #
    ##########################
    def clear(self, int length=-1):
        """ Writes zeros into the buffer, adjusting the
            size if length > 0.
        """
        if length <= 0:
            self.frames = None
        else:
            self.frames = np.zeros((length, self.channels))

    def cloud(self, double length=-1, *args, **kwargs):
        """ Create a new GrainCloud from this SoundBuffer
        """
        if length <= 0:
            length = self.dur
        return grains.GrainCloud(self, *args, **kwargs).play(length)

    def copy(self):
        """ Return a new copy of this SoundBuffer.
        """
        if self.frames is None:
            return SoundBuffer(channels=self.channels, samplerate=self.samplerate)
        return SoundBuffer(np.array(self.frames, copy=True), self.channels, self.samplerate)

    def clip(self, minval=-1, maxval=1):
        return SoundBuffer(np.clip(self.frames, minval, maxval), self.channels, self.samplerate)
        
    def cut(self, double start=0, double length=1):
        """ Copy a portion of this soundbuffer, returning 
            a new soundbuffer with the selected slice.

            This is called `cut` for historical reasons, but it 
            might be better understood if renamed to `slice` as it 
            doesn't modify the source SoundBuffer in any way.
            
            The `start` param is a position in seconds to begin 
            cutting, and the `length` param is the cut length in seconds.

            Overflowing values that exceed the boundries of the source SoundBuffer 
            will return a SoundBuffer padded with silence so that the `length` param 
            is always respected.
        """
        cdef int readstart = <int>(start * self.samplerate)
        cdef int outlength = <int>(length * self.samplerate)
        cdef int readend = outlength + readstart
        cdef int readlength = len(self) - readstart

        cdef double[:,:] out = np.zeros((outlength, self.channels))
        if readlength >= 0:
            out[0:readlength] = self.frames[readstart:readend]

        return SoundBuffer(out, self.channels, self.samplerate)

    def rcut(self, double length=1):
        """ Copy a portion of this SoundBuffer of the 
            given length in seconds starting from a random 
            position within it. 
            
            This will always return a complete SoundBuffer 
            without overflows or added silence, and the entire 
            sound will be returned without added silence if a length 
            that exceeds the length of the source SoundBuffer is given -- 
            unlike SoundBuffer.cut() which will pad results with silence 
            to preserve the length param if an invalid or overflowing offset 
            position is given.
        """
        cdef double maxlen = self.dur - length
        if maxlen <= 0:
            return self
        cdef double start = random.triangular(0, maxlen)
        return self.cut(start, length)

    cdef void _dub(self, SoundBuffer sound, int framepos=0):
        cdef int dubsnd_length = len(sound.frames)
        cdef int source_length = len(self.frames)
        cdef int total_length = framepos + dubsnd_length
        if self.frames is None or source_length == 0:
            self.frames = np.zeros((total_length, self.channels), dtype='float64')

        elif source_length < total_length:
            self.frames = np.vstack((
                    self.frames.copy(), 
                    np.zeros((total_length - source_length, self.channels), dtype='float64')
                ))

        #cdef int i = 0
        #cdef int c = 0
        #for i in range(dubsnd_length):
        #    for c in range(self.channels):
        #        self.frames[framepos+i][c] += sound[i][c]

        self.frames.base[framepos:total_length] += sound.frames

    def dub(self, sounds, double pos=-1, int framepos=0):
        """ Dub a sound or iterable of sounds into this soundbuffer
            starting at the given position in fractional seconds.
            >>> snd.dub(snd2, 3.2)

            To dub starting at a specific frame position use:
            >>> snd.dub(snd3, framepos=111)
        """
        cdef int numsounds
        cdef int sound_index

        if pos >= 0:
            framepos = <int>(pos * self.samplerate)

        if isinstance(sounds, SoundBuffer):
            self._dub(sounds, framepos)
        else:
            numsounds = len(sounds)
            try:
                for sound_index in range(numsounds):
                    self._dub(sounds[sound_index], framepos)

            except TypeError as e:
                raise TypeError('Please provide a SoundBuffer or list of SoundBuffers for dubbing') from e

    def env(self, int window_type=-1):
        """ Apply an amplitude envelope 
            to the sound of the given type.

            To modulate a sound with an arbitrary 
            iterable, simply do:

            >>> snd * iterable

            Where iterable is a list, array, or SoundBuffer with 
            the same # of channels and of any length
        """
        cdef int length = len(self)
        cdef double[:] wavetable = None
        wavetable = wavetables._window(window_type, length)

        if wavetable is None:
            raise TypeError('Invalid envelope type')

        return self * wavetable

    @cython.boundscheck(False)
    @cython.wraparound(False)
    cdef void _fill(self, double[:,:] frames):
        """ Fill current w/frames
        """
        cdef int frame_index, channel_index = 0
        cdef int framelen = len(frames)
        cdef int length = len(self.frames)
        cdef int minlen = min(framelen, length)

        for frame_index in range(minlen):
            for channel_index in range(self.channels):
                self.frames[frame_index][channel_index] = frames[frame_index][channel_index]

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

    def grains(self, double minlength, double maxlength=-1):
        """ Iterate over the buffer in fixed-size grains.
            If a second length is given, iterate in randomly-sized 
            grains, given the minimum and maximum sizes.
        """
        cdef int framesread = 0
        cdef int maxframes = <int>(maxlength * self.samplerate)
        cdef int minframes = <int>(minlength * self.samplerate)
        cdef int grainlength = minframes
        cdef int sourcelength = len(self)
        while framesread < sourcelength:
            if minlength > 0 and maxlength > 0:
                grainlength = random.randint(minframes, maxframes)

            yield self[framesread:framesread+grainlength]

            framesread += grainlength

    def max(self):
        return np.amax(self.frames)
 
    def mix(self, sounds):
        """ Mix this sound in place with a sound or iterable of sounds
        """
        cdef SoundBuffer sound
        cdef int numsounds
        cdef int sound_index
        if isinstance(sounds, SoundBuffer):
            self &= sounds
        else:
            numsounds = len(sounds)
            try:
                for sound_index in range(numsounds):
                    self &= sounds[sound_index] 
            except TypeError as e:
                raise TypeError('Please provide a SoundBuffer or list of SoundBuffers for mixing') from e

    def pad(self, double start=0, double end=0):
        """ Pad this sound with silence at start or end
        """
        if start <= 0 and end <= 0: 
            return self

        cdef int framestart = <int>(start * self.samplerate)
        cdef int frameend = <int>(end * self.samplerate)

        cdef double[:,:] out

        if framestart > 0:
            out = np.vstack((np.zeros((framestart, self.channels)), self.frames))

        if frameend > 0:
            out = np.vstack((self.frames, np.zeros((frameend, self.channels))))
 
        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

    def pan(self, double pos=0.5, unicode method=None, int start=0):
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
        cdef int channel = 0

        if method == 'constant':
            for channel in range(self.channels):
                if channel % 2 == 0:
                    out.base[:,channel] *= math.sqrt(pos)
                else:
                    out.base[:,channel] *= math.sqrt(1 - pos)

        elif method == 'linear':
            for channel in range(self.channels):
                if channel % 2 == 0:
                    out.base[:,channel] *= pos
                else:
                    out.base[:,channel] *= 1 - pos

        elif method == 'sine':
            for channel in range(self.channels):
                if channel % 2 == 0:
                    out.base[:,channel] *= math.sin(pos * (math.pi / 2))
                else:
                    out.base[:,channel] *= math.cos(pos * (math.pi / 2))

        elif method == 'gogins':
            for channel in range(self.channels):
                if channel % 2 == 0:
                    out.base[:,channel] *= math.sin((pos + 0.5) * (math.pi / 2))
                else:
                    out.base[:,channel] *= math.cos((pos + 0.5) * (math.pi / 2))

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

    def remix(self, int channels):
        return SoundBuffer(_remix(self.frames, len(self.frames), channels), self.samplerate, channels)

    def repeat(self, int reps=2):
        if reps <= 1:
            return SoundBuffer(self.frames)

        out = SoundBuffer(self.frames)
        for _ in range(reps-1):
            out += self

        return out

    def reverse(self):
        self.frames = np.flip(self.frames, 0)

    def reversed(self):
        return SoundBuffer(np.flip(self.frames, 0), channels=self.channels, samplerate=self.samplerate)

    cdef double[:,:] _speed(self, double speed):
        speed = speed if speed > 0 else 0.001
        cdef int length = <int>(len(self) * (1.0 / speed))
        cdef double[:,:] out = np.zeros((length, self.channels))
        cdef int channel

        for channel in range(self.channels):
            out[:,channel] = interpolation._linear(np.asarray(self.frames[:,channel]), length)

        return out

    def speed(self, double speed):
        """ Change the speed of the sound
        """
        return SoundBuffer(self._speed(speed), channels=self.channels, samplerate=self.samplerate)

    def stretch(self, double length, double grainlength=60):
        """ Change the length of the sound without changing the pitch.
            Granular-only implementation at the moment.
        """
        return grains.GrainCloud(self, grainlength=grainlength).play(length)

    def taper(self, double length):
        cdef int framelength = <int>(self.samplerate * length)
        return self * wavetables._adsr(len(self), framelength, 0, 1, framelength)

    def transpose(self, double speed, double grainlength=60):
        """ Change the pitch of the sound without changing the length.
            This is just a wrapper for `GrainCloud` with `speed` and `grainlength` 
            settings exposed, returning a cloud equal in length to the source sound.

            TODO accept: from/to hz, notes, midi notes, intervals
        """
        return grains.GrainCloud(self, 
                speed=speed, 
                grainlength=grainlength, 
            ).play(self.dur)

    def write(self, unicode filename=None):
        """ Write the contents of this buffer to disk 
            in the given audio file format. (WAV, AIFF, AU)
        """
        return soundfile.write(filename, self.frames, self.samplerate)


cdef class RingBuffer:
    """ Simple fixed-length ring buffer, lacking 
        the transformations and operator 
        overloading found in SoundBuffer but 
        useful for loopers, delay lines, etc.
        Also, this always speaks in frame lengths rather 
        than time units expressed in seconds.
        It's mostly just for the astrid input sampler right now.
    """
    def __cinit__(self, int length, int channels=DEFAULT_CHANNELS, int samplerate=DEFAULT_SAMPLERATE, double[:,:] frames=None):
        self.samplerate = samplerate
        self.channels = channels
        self.length = length

        if frames is not None:
            self.frames = frames
        else:
            self.frames = np.zeros((length, channels))

        self.copyout = np.zeros((length, channels))
        self.write_head = 0

    def __reduce__(self):
        return (rebuild_ringbuffer, (self.length, self.channels, self.samplerate, self.frames))

    def __len__(self):
        return self.length

    @cython.boundscheck(False)
    @cython.wraparound(False)
    cdef double[:,:] _read(self, int length, int offset):
        """ Read a length of frames from the buffer
            where length is at maximum the length of 
            the buffer, always returning most recently 
            written frames.
        """
        cdef int i = 0
        cdef int read_head = 0

        if length > self.length:
            length = self.length

        read_head = (self.write_head - length + offset) % self.length
        for i in range(length):
            self.copyout[i] = self.frames[read_head]
            read_head = (read_head + 1) % self.length

        return self.copyout[:length]

    def read(self, int length, int offset=0):
        return self._read(length, offset)

    @cython.boundscheck(False)
    @cython.wraparound(False)
    cdef void _write(self, double[:,:] frames):
        """ Write given frames into the buffer, advancing 
            the write head and wrapping as needed.
        """
        cdef int i = 0
        cdef int inputlength = len(frames)
        
        # write forward in the buffer, from the current
        # write head position, until all frames are written
        # wrapping the write head at the boundry of the buffer
        for i in range(inputlength):
            self.frames[self.write_head] = frames[i]
            self.write_head = (self.write_head + 1) % self.length

    def write(self, double[:,:] frames):
        self._write(frames)


cpdef object rebuild_buffer(double[:,:] frames, int channels, int samplerate):
    return SoundBuffer(frames, channels=channels, samplerate=samplerate)

cpdef object rebuild_ringbuffer(int length, int channels, int samplerate, double[:,:] frames):
    return RingBuffer(length, channels=channels, samplerate=samplerate, frames=frames)


