#cython: language_level=3

from cpython cimport Py_buffer
from libc.stdint cimport uintptr_t
import numbers
import warnings

cimport cython
from cpython.array cimport array, clone
cimport numpy as np
import numpy as np
from pysndfile import sndio

from pippi.defaults cimport DEFAULT_WTSIZE, DEFAULT_CHANNELS, DEFAULT_SAMPLERATE
from pippi cimport fft
from pippi import graph, rand
from pippi.wavetables cimport _window, Wavetable
from pippi cimport interpolation

cdef dict FLAGS = {
    'sine': WIN_SINE,
    'sineine': WIN_SINEIN,
    'sineout': WIN_SINEOUT,
    'cos': WIN_COS,
    'tri': WIN_TRI, 
    'phasor': WIN_PHASOR, 
    'hann': WIN_HANN, 
    'hannin': WIN_HANNIN, 
    'hannout': WIN_HANNOUT, 
    'rnd': WIN_RND,
    'saw': WIN_SAW,
    'rsaw': WIN_RSAW,
}

cdef int to_flag(str name):
    try:
        return FLAGS[name]
    except KeyError:
        return WIN_SINE

cdef lpbuffer_t * to_window(object o, size_t length=0):
    cdef lpbuffer_t * out
    if length <= 0:
        length = DEFAULT_WTSIZE

    if isinstance(o, str):
        out = LPWindow.create(to_flag(o), length)

    elif not isinstance(o, SoundBuffer):
        out = to_lpbuffer(o, length)

    return out

cdef lpbuffer_t * to_lpbuffer(object o, size_t length):
    cdef lpbuffer_t * out
    cdef size_t i
    cdef int channels = DEFAULT_CHANNELS

    assert(o is not None)

    if isinstance(o, numbers.Real):
        out = LPBuffer.create_from_float(<lpfloat_t>o, length, DEFAULT_CHANNELS, DEFAULT_SAMPLERATE)

    elif isinstance(o, Wavetable):
        out = LPBuffer.create(length, 1, DEFAULT_SAMPLERATE)
        for i in range(length):
            out.data[i] = o[i]

    elif isinstance(o, SoundBuffer):
        return (<SoundBuffer>o).buffer

    elif isinstance(o, list):
        out = LPBuffer.create(length, channels, DEFAULT_SAMPLERATE)
        if isinstance(o[0], list):
            channels = len(o[0])
            for i in range(length):
                for c in range(channels):
                    out.data[i * channels + c] = o[i][c]
        else:
            for i in range(length):
                out.data[i] = <lpfloat_t>o[i]

    elif isinstance(o, np.ndarray):
        channels = o.shape[1]
        out = LPBuffer.create(length, channels, DEFAULT_SAMPLERATE)
        for i in range(length):
            for c in range(channels):
                out.data[i * channels + c] = o[i * channels + c]

    else:
        raise NotImplemented

    return out


class SoundBufferError(Exception):
    pass


@cython.final
@cython.total_ordering
cdef class SoundBuffer:
    cdef lpbuffer_t * buffer
    cdef Py_ssize_t shape[2]
    cdef Py_ssize_t strides[2]

    def __cinit__(SoundBuffer self, 
            object frames=None, 
            double length=-1, 
            int channels=DEFAULT_CHANNELS, 
            int samplerate=DEFAULT_SAMPLERATE, 
            str filename=None, 
            double start=0, 
            double[:,:] buf=None
        ):

        cdef size_t framelength = 0

        if filename is not None:
            # Filename will always override frames input
            frames, samplerate, _ = sndio.read(filename, None if framelength==0 else framelength, start * samplerate, dtype=np.float64, force_2d=True)
            channels = frames.shape[1]

        if frames is None and length > 0:
            # Create a silent buffer of the given length
            framelength = <size_t>(length * samplerate)
            self.buffer = LPBuffer.create(framelength, channels, samplerate)

        elif frames is not None:
            # Try to fill the buffer from the given frames object
            try:
                frames = np.asarray(frames, dtype='d')
                framelength = len(frames)
                channels = 1
                if len(frames.shape) > 1: 
                    channels = frames.shape[1] 

                self.buffer = LPBuffer.create(framelength, channels, samplerate)
                if channels == 1:
                    for i in range(framelength):
                        self.buffer.data[i] = frames[i]

                else:
                    for i in range(framelength):
                        for c in range(channels):
                            self.buffer.data[i * channels + c] = frames[i][c]

            except Exception as e:
                raise SoundBufferError('Invalid source for SoundBuffer. Got frames of type %s' % type(frames)) from e

        else:
            # A null buffer has 0 frames and is falsey
            self.buffer = NULL

    property channels:
        def __get__(SoundBuffer self):
            if self.buffer != NULL:
                return self.buffer.channels
            return 0

    property samplerate:
        def __get__(SoundBuffer self):
            if self.buffer != NULL:
                return self.buffer.samplerate
            return DEFAULT_SAMPLERATE

    property length:
        def __get__(SoundBuffer self):
            if self.buffer != NULL:
                return self.buffer.length
            return 0

    property dur:
        def __get__(SoundBuffer self):
            if self.buffer != NULL:
                return <double>self.buffer.length / <double>self.samplerate
            return 0

    property frames:
        def __get__(SoundBuffer self):
            if self.buffer != NULL:
                return memoryview(self)
            return None

    property min:
        def __get__(SoundBuffer self):
            if self.buffer != NULL:
                return <double>LPBuffer.min(self.buffer)
            return 0

    property max:
        def __get__(SoundBuffer self):
            if self.buffer != NULL:
                return <double>LPBuffer.max(self.buffer)
            return 0

    property mag:
        def __get__(SoundBuffer self):
            if self.buffer != NULL:
                return <double>LPBuffer.mag(self.buffer)
            return 0

    @staticmethod
    cdef SoundBuffer fromlpbuffer(lpbuffer_t * buffer):
        cdef SoundBuffer out = SoundBuffer.__new__(SoundBuffer, channels=buffer.channels, samplerate=buffer.samplerate)
        out.buffer = buffer
        return out

    def __bool__(self):
        return bool(len(self))

    def __dealloc__(SoundBuffer self):
        LPBuffer.destroy(self.buffer)

    def __repr__(self):
        return 'SoundBuffer(samplerate=%s, channels=%s, frames=%s, dur=%.2f)' % (self.samplerate, self.channels, len(self.frames), self.dur)

    def __ge__(self, other):
        if not isinstance(other, SoundBuffer):
            return NotImplemented
        return self.buffer.length >= (<SoundBuffer>other).buffer.length

    def __eq__(self, other):
        if not isinstance(other, SoundBuffer):
            return False
        return LPBuffer.buffers_are_close(self.buffer, (<SoundBuffer>other).buffer, 1000) != 0

    def __getbuffer__(SoundBuffer self, Py_buffer * buffer, int flags):
        cdef Py_ssize_t itemsize = sizeof(self.buffer.data[0])
        self.shape[0] = <Py_ssize_t>self.buffer.length
        self.shape[1] = <Py_ssize_t>self.buffer.channels
        self.strides[1] = <Py_ssize_t>(<char *>&(self.buffer.data[1]) - <char *>&(self.buffer.data[0]))
        self.strides[0] = self.buffer.channels * self.strides[1]

        buffer.buf = <char *>&(self.buffer.data[0])
        buffer.format = 'd'
        buffer.internal = NULL
        buffer.itemsize = itemsize
        buffer.len = self.buffer.length * self.buffer.channels
        buffer.ndim = 2
        buffer.obj = self
        buffer.readonly = 0
        buffer.shape = self.shape
        buffer.strides = self.strides
        buffer.suboffsets = NULL

    def __releasebuffer__(SoundBuffer self, Py_buffer * buffer):
        pass

    def __getitem__(self, position):
        cdef double[:,:] mv = memoryview(self)
        if position >= self.buffer.length:
            raise IndexError('Cannot read index %s from buffer of length %s' % (position, self.buffer.length))
        return tuple([ mv[position][v] for v in range(self.channels) ])

    def __len__(self):
        return 0 if self.buffer == NULL else <int>self.buffer.length

    def __add__(SoundBuffer self, object value):
        cdef Py_ssize_t i, c
        cdef lpbuffer_t * data
        cdef SoundBuffer tmp

        if self.buffer == NULL:
            if isinstance(value, numbers.Real):
                return SoundBuffer(channels=self.channels, samplerate=self.samplerate)

            elif isinstance(value, SoundBuffer):
                return value.copy()

            else:
                return SoundBuffer(value, channels=self.channels, samplerate=self.samplerate)

        if isinstance(value, numbers.Real):
            data = LPBuffer.create(self.buffer.length, self.buffer.channels, self.buffer.samplerate)
            LPBuffer.copy(self.buffer, data)
            LPBuffer.add_scalar(data, <lpfloat_t>value)

        elif isinstance(value, SoundBuffer):
            data = LPBuffer.concat(self.buffer, (<SoundBuffer>value).buffer)

        else:
            try:
                tmp = SoundBuffer(value)
                data = LPBuffer.concat(self.buffer, tmp.buffer)
            except Exception as e:
                return NotImplemented

        return SoundBuffer.fromlpbuffer(data)

    def __iadd__(SoundBuffer self, object value):
        cdef Py_ssize_t i, c

        if self.buffer == NULL:
            if isinstance(value, numbers.Real):
                return self

            elif isinstance(value, SoundBuffer):
                return value.copy()

            else:
                return SoundBuffer(value, channels=self.channels, samplerate=self.samplerate)


        if isinstance(value, numbers.Real):
            LPBuffer.add_scalar(self.buffer, <lpfloat_t>value)

        elif isinstance(value, SoundBuffer):
            LPBuffer.add(self.buffer, (<SoundBuffer>value).buffer)

        else:
            try:
                tmp = SoundBuffer(value)
                LPBuffer.add(self.buffer, tmp.buffer)
            except Exception as e:
                return NotImplemented

        return self

    def __radd__(SoundBuffer self, object value):
        return self + value

    def __sub__(SoundBuffer self, object value):
        cdef Py_ssize_t i, c
        cdef lpbuffer_t * data
        cdef SoundBuffer tmp

        if self.buffer == NULL:
            if isinstance(value, numbers.Real):
                return SoundBuffer(channels=self.channels, samplerate=self.samplerate)

            elif isinstance(value, SoundBuffer):
                return value.copy()

            else:
                return SoundBuffer(value, channels=self.channels, samplerate=self.samplerate)

        data = LPBuffer.create(self.buffer.length, self.buffer.channels, self.buffer.samplerate)
        LPBuffer.copy(self.buffer, data)

        if isinstance(value, numbers.Real):
            LPBuffer.subtract_scalar(data, <lpfloat_t>value)

        elif isinstance(value, SoundBuffer):
            LPBuffer.subtract(data, (<SoundBuffer>value).buffer)

        else:
            try:
                tmp = SoundBuffer(value)
                LPBuffer.subtract(data, tmp.buffer)
            except Exception as e:
                return NotImplemented

        return SoundBuffer.fromlpbuffer(data)

    def __isub__(SoundBuffer self, object value):
        cdef Py_ssize_t i, c

        if self.buffer == NULL:
            if isinstance(value, numbers.Real):
                return self

            elif isinstance(value, SoundBuffer):
                return value.copy()

            else:
                return SoundBuffer(value, channels=self.channels, samplerate=self.samplerate)

        if isinstance(value, numbers.Real):
            LPBuffer.subtract_scalar(self.buffer, <lpfloat_t>value)

        elif isinstance(value, SoundBuffer):
            LPBuffer.subtract(self.buffer, (<SoundBuffer>value).buffer)

        else:
            try:
                tmp = SoundBuffer(value)
                LPBuffer.subtract(self.buffer, tmp.buffer)
            except Exception as e:
                return NotImplemented

        return self

    def __rsub__(SoundBuffer self, object value):
        return self - value 

    def __mul__(SoundBuffer self, object value):
        cdef Py_ssize_t i, c
        cdef lpbuffer_t * data = LPBuffer.create(self.buffer.length, self.buffer.channels, self.buffer.samplerate)
        LPBuffer.copy(self.buffer, data)

        if isinstance(value, numbers.Real):
            LPBuffer.multiply_scalar(data, <lpfloat_t>value)

        elif isinstance(value, SoundBuffer):
            LPBuffer.multiply(data, (<SoundBuffer>value).buffer)

        else:
            try:
                for i in range(self.buffer.length):
                    for c in range(self.buffer.channels):
                        data.data[i * self.buffer.channels + c] *= value[i * self.buffer.channels + c]

            except IndexError as e:
                pass

            except Exception as e:
                return NotImplemented

        return SoundBuffer.fromlpbuffer(data)

    def __imul__(SoundBuffer self, object value):
        cdef Py_ssize_t i, c

        if isinstance(value, numbers.Real):
            LPBuffer.multiply_scalar(self.buffer, <lpfloat_t>value)

        elif isinstance(value, SoundBuffer):
            LPBuffer.multiply(self.buffer, (<SoundBuffer>value).buffer)

        else:
            try:
                for i in range(self.buffer.length):
                    for c in range(self.buffer.channels):
                        self.buffer.data[i * self.buffer.channels + c] *= value[i * self.buffer.channels + c]

            except IndexError as e:
                pass

            except Exception as e:
                return NotImplemented

        return self

    def __rmul__(SoundBuffer self, object value):
        return self * value

    def __and__(SoundBuffer self, object value):
        if not isinstance(value, SoundBuffer):
            return NotImplemented

        cdef lpbuffer_t * out

        out = LPBuffer.mix(self.buffer, (<SoundBuffer>value).buffer)
        return SoundBuffer.fromlpbuffer(out)

    def __iand__(SoundBuffer self, object value):
        if not isinstance(value, SoundBuffer):
            return NotImplemented

        self.buffer = LPBuffer.mix(self.buffer, (<SoundBuffer>value).buffer)
        return self

    def __truediv__(SoundBuffer self, object value):
        cdef Py_ssize_t i, c
        cdef lpbuffer_t * data
        cdef SoundBuffer tmp

        if self.buffer == NULL:
            if isinstance(value, numbers.Real):
                return SoundBuffer(channels=self.channels, samplerate=self.samplerate)

            elif isinstance(value, SoundBuffer):
                return value.copy()

            else:
                return SoundBuffer(value, channels=self.channels, samplerate=self.samplerate)

        data = LPBuffer.create(self.buffer.length, self.buffer.channels, self.buffer.samplerate)
        LPBuffer.copy(self.buffer, data)

        if isinstance(value, numbers.Real):
            LPBuffer.divide_scalar(data, <lpfloat_t>value)

        elif isinstance(value, SoundBuffer):
            LPBuffer.divide(data, (<SoundBuffer>value).buffer)

        else:
            try:
                tmp = SoundBuffer(value)
                LPBuffer.divide(data, (<SoundBuffer>value).buffer)
            except Exception as e:
                return NotImplemented

        return SoundBuffer.fromlpbuffer(data)

    def __itruediv__(SoundBuffer self, object value):
        cdef Py_ssize_t i, c

        if self.buffer == NULL:
            if isinstance(value, numbers.Real):
                return self

            elif isinstance(value, SoundBuffer):
                return value.copy()

            else:
                return SoundBuffer(value, channels=self.channels, samplerate=self.samplerate)

        if isinstance(value, numbers.Real):
            LPBuffer.divide_scalar(self.buffer, <lpfloat_t>value)

        elif isinstance(value, SoundBuffer):
            LPBuffer.divide(self.buffer, (<SoundBuffer>value).buffer)

        else:
            try:
                tmp = SoundBuffer(value)
                LPBuffer.divide(self.buffer, tmp.buffer)
            except Exception as e:
                return NotImplemented

        return self

    def __rtruediv__(SoundBuffer self, object value):
        return self / value 

    def clear(SoundBuffer self):
        LPBuffer.clear(self.buffer)
        return self

    cpdef SoundBuffer convolve(SoundBuffer self, SoundBuffer impulse, bint norm=True):
        cdef lpbuffer_t * out

        if not isinstance(impulse, SoundBuffer):
            raise TypeError('Could not convolve impulse of type %s' % type(impulse))

        if impulse.channels != self.channels:
            impulse = impulse.remix(self.channels)

        out = LPSpectral.convolve(self.buffer, impulse.buffer)

        return SoundBuffer.fromlpbuffer(out)

    def copy(SoundBuffer self):
        cdef lpbuffer_t * out
        out = LPBuffer.create(self.buffer.length, self.buffer.channels, self.buffer.samplerate)
        LPBuffer.copy(self.buffer, out)
        return SoundBuffer.fromlpbuffer(out)

    def cut(SoundBuffer self, double start=0, double length=1):
        """ Copy a portion of this soundbuffer, returning 
            a new soundbuffer with the selected slice.
           
            The `start` param is a position in seconds to begin 
            cutting, and the `length` param is the cut length in seconds.

            Overflowing values that exceed the boundries of the source SoundBuffer 
            will return a SoundBuffer padded with silence so that the `length` param 
            is always respected.
        """
        cdef size_t readstart = <size_t>(start * self.samplerate)
        cdef size_t outlength = <size_t>(length * self.samplerate)
        cdef lpbuffer_t * out
        out = LPBuffer.cut(self.buffer, readstart, outlength)
        return SoundBuffer.fromlpbuffer(out)

    def fcut(SoundBuffer self, size_t start=0, size_t length=1):
        """ Copy a portion of this soundbuffer, returning 
            a new soundbuffer with the selected slice.

            Identical to `cut` except `start` and `length` 
            should be given in frames instead of seconds.
        """
        cdef lpbuffer_t * out
        out = LPBuffer.cut(self.buffer, start, length)
        return SoundBuffer.fromlpbuffer(out)

    def rcut(SoundBuffer self, double length=1):
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
        cdef double start = rand.rand(0, maxlen)
        return self.cut(start, length)

    def dub(SoundBuffer self, object sounds, double pos=0, size_t framepos=0):
        """ Dub a sound or iterable of sounds into this soundbuffer
            starting at the given position in fractional seconds.

                >>> snd.dub(snd2, 3.2)

            To dub starting at a specific frame position use:

                >>> snd.dub(snd3, framepos=111)
        """
        cdef int numsounds, i

        if pos > 0:
            framepos = <size_t>(pos * self.samplerate)

        return self.fdub(sounds, framepos)

    cpdef SoundBuffer fdub(SoundBuffer self, object sounds, size_t framepos=0):
        if isinstance(sounds, SoundBuffer):
            LPBuffer.dub(self.buffer, (<SoundBuffer>sounds).buffer, framepos)

        elif isinstance(sounds, numbers.Real):
            LPBuffer.dub_scalar(self.buffer, <lpfloat_t>sounds, framepos)

        else:
            numsounds = len(sounds)
            try:
                for i in range(numsounds):
                    LPBuffer.dub(self.buffer, (<SoundBuffer>sounds[i]).buffer, framepos)

            except TypeError as e:
                raise TypeError('Please provide a SoundBuffer or list of SoundBuffers for dubbing') from e

        return self

    cpdef SoundBuffer remix(SoundBuffer self, int channels):
        channels = max(channels, 1)
        cdef lpbuffer_t * out = LPBuffer.remix(self.buffer, channels)
        return SoundBuffer.fromlpbuffer(out)

    cpdef SoundBuffer repeat(SoundBuffer self, size_t repeats):
        repeats = max(repeats, 1)
        cdef lpbuffer_t * out = LPBuffer.repeat(self.buffer, repeats)
        return SoundBuffer.fromlpbuffer(out)

    cpdef reverse(SoundBuffer self):
        self.buffer = <lpbuffer_t *>LPBuffer.reverse(self.buffer)

    cpdef SoundBuffer reversed(SoundBuffer self):
        cdef lpbuffer_t * out = LPBuffer.reverse(self.buffer)
        return SoundBuffer.fromlpbuffer(out)

    cpdef SoundBuffer env(SoundBuffer self, object window=None):
        """ Apply an amplitude envelope 
            to the sound of the given type.

            To modulate a sound with an arbitrary 
            iterable, simply do:

                >>> snd * iterable

            Where iterable is a list, array, or SoundBuffer with 
            the same # of channels and of any length
        """
        cdef lpbuffer_t * w
        cdef lpbuffer_t * out
        if window is None:
            window = 'sine'
        cdef int length = len(self)
        out = LPBuffer.create(length, self.channels, self.samplerate)
        LPBuffer.copy(self.buffer, out)
        w = to_window(window, length)
        LPBuffer.multiply(out, w)
        return SoundBuffer.fromlpbuffer(out)

    def graph(SoundBuffer self, *args, **kwargs):
        return graph.write(self, *args, **kwargs)

    def plot(SoundBuffer self):
        LPBuffer.plot(self.buffer)

    def write(self, unicode filename=None):
        """ Write the contents of this buffer to disk 
            in the given audio file format. (WAV, AIFF, AU)
        """
        if filename.endswith('.wav'):
            LPSoundFile.write(filename.encode('ascii'), self.buffer)
        else:
            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
            sndio.write(filename, np.asarray(self), self.samplerate)

