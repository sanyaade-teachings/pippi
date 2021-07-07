#cython: language_level=3

from cpython cimport Py_buffer
from libc.stdint cimport uintptr_t
import numbers

cimport cython
cimport numpy as np
import numpy as np
import soundfile

DEFAULT_CHANNELS = 2
DEFAULT_SAMPLERATE = 44100

class SoundBufferError(Exception):
    pass

@cython.final
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
            frames, samplerate = soundfile.read(filename, framelength, start * samplerate, dtype='float64', fill_value=0, always_2d=True)
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
                for i in range(framelength):
                    for c in range(channels):
                        self.buffer.data[i * channels + c] = frames[i * channels + c]

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

    @staticmethod
    cdef SoundBuffer fromlpbuffer(lpbuffer_t * buffer):
        cdef SoundBuffer out = SoundBuffer.__new__(SoundBuffer)
        out.buffer = buffer
        return out

    ################################
    #        DUNDERVILLE           #
    #                              # 
    # These dunder methods define  #
    # the SoundBuffer data model.  #
    #                              #
    # See notes below.             #
    ################################
    def __bool__(self):
        return bool(len(self))

    def __dealloc__(SoundBuffer self):
        LPBuffer.destroy(self.buffer)

    def __repr__(self):
        return 'SoundBuffer(samplerate=%s, channels=%s, frames=%s, dur=%.2f)' % (self.samplerate, self.channels, len(self.frames), self.dur)

    def __eq__(self, other):
        if not isinstance(other, SoundBuffer):
            return False
        return LPBuffer.buffers_are_equal(self.buffer, (<SoundBuffer>other).buffer) != 0

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
        return self.frames[position]

    def __len__(self):
        return 0 if self.buffer == NULL else <int>self.buffer.length

    #############################################################
    # (+) Addition / concatenation operator                     #
    #                                                           #
    # - Adding two soundbuffers will concatenate them.          #
    #                                                           #
    # - Adding a soundbuffer to a list-like type will           #
    #   convert the list into a soundbuffer and concatenate     #
    #   them.                                                   #
    #                                                           #
    # - Adding a numeric value to a soundbuffer will            #
    #   add the value to each sample of each channel of         #
    #   the soundbuffer.                                        #
    #############################################################
    """
    def __add__(SoundBuffer self, object value):
        # Adding something to an empty buffer
        if self.buffer == NULL:
            if isinstance(value, numbers.Real):
                # Numeric operands return an empty SoundBuffer
                return SoundBuffer(channels=self.channels, samplerate=self.samplerate)

            elif isinstance(value, SoundBuffer):
                # SoundBuffer operands return a copy of themselves
                return value.copy()

            else:
                # Other types try to return a SoundBuffer created from the data
                return SoundBuffer(value, channels=self.channels, samplerate=self.samplerate)

        cdef double[:,:] out = np.zeros((sef.buffer.length, self.buffer.channels))

        if isinstance(value, numbers.Real):
            out = np.add(self.frames, value)
        elif isinstance(value, SoundBuffer):
            if value.channels != self.channels:
                value = value.remix(self.channels)

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

    def __iadd__(SoundBuffer self, object value):
        if self.frames is None:
            if isinstance(value, numbers.Real):
                return self
            elif isinstance(value, SoundBuffer):
                return value.copy()
            else:
                return SoundBuffer(value, channels=self.channels, samplerate=self.samplerate)

        cdef SoundBuffer out

        if isinstance(value, numbers.Real):
            self.frames = np.add(self.frames, value)
        elif isinstance(value, SoundBuffer):
            if value.channels != self.channels:
                value = value.remix(self.channels)
            self.dub(value, self.dur)
        else:
            try:
                self.frames = np.vstack((self.frames, value))
            except TypeError as e:
                return NotImplemented

        return self

    def __radd__(SoundBuffer self, object value):
        return self + value

    """

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


