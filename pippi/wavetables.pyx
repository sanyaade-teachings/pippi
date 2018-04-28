# cython: language_level=3

import collections
import random
import numbers

import soundfile
import numpy as np
import re

from libc.stdlib cimport malloc, realloc, calloc, free
from libc.stdlib cimport rand
from libc cimport math

from . cimport interpolation
from .soundbuffer cimport SoundBuffer

cdef int SINE = 0
cdef int COS = 1
cdef int TRI = 2
cdef int SAW = 3
cdef int PHASOR = SAW
cdef int RSAW = 4
cdef int HANN = 5
cdef int HAMM = 6
cdef int BLACK = 7
cdef int BLACKMAN = 7
cdef int BART = 8
cdef int BARTLETT = 8
cdef int KAISER = 9
cdef int SQUARE = 10
cdef int RND = 11

cdef dict wtype_flags = {
    'sine': SINE, 
    'cos': COS, 
    'tri': TRI, 
    'saw': SAW, 
    'phasor': PHASOR, 
    'rsaw': RSAW, 
    'hann': HANN, 
    'hamm': HAMM, 
    'black': BLACK, 
    'blackman': BLACKMAN, 
    'bart': BART, 
    'bartlett': BARTLETT, 
    'kaiser': KAISER, 
    'rnd': RND, 
}

cdef int LEN_WINDOWS = 9
cdef int *ALL_WINDOWS = [
            SINE, 
            TRI, 
            SAW,
            RSAW,
            HANN,
            HAMM,
            BLACK,
            BART,
            KAISER
        ]

cdef int LEN_WAVETABLES = 6
cdef int *ALL_WAVETABLES = [
            SINE, 
            COS,
            TRI,
            SAW,
            RSAW,
            SQUARE
        ]

cdef double SQUARE_DUTY = 0.5

SEGMENT_RE = re.compile('(?P<length>0?\.?\d+)?,?(?P<wtype>\w+),?(?P<start>0?\.?\d+)?-?(?P<end>0?\.?\d+)?')

cdef int wtypentf(str wtype_name):
    return wtype_flags[wtype_name]


cdef class Wavetable:
    def __cinit__(self, list values=None, int initwt=-1, int initwin=-1, int length=-1):
        if length < 0:
            length = 4096

        if values is not None:
            self.data = np.array(values)
        elif initwt < 0:
            self.data = _wavetable(initwt, length)
        elif initwin < 0:
            self.data = _window(initwt, length)
        else:
            self.data = None


    #############################################
    # (+) Addition & concatenation operator (+) #
    #############################################
    def __add__(self, value):
        """ Add a number to every sample in this SoundBuffer or concatenate with another 
            SoundBuffer or iterable with compatible dimensions
        """
        cdef int length = len(self.data)
        cdef double[:] out = np.zeros(length)

        if isinstance(value, numbers.Real):
            out = np.add(self.data, value)
        elif isinstance(value, Wavetable):
            out = np.add(self.data, value.data)
            pass
        elif isinstance(value, SoundBuffer):
            out = np.add(self.data, value.tomono().frames)
        else:
            try:
                self.data = np.hstack((self.data, value))
            except TypeError as e:
                return NotImplemented

        return Wavetable(out)

    def __iadd__(self, value):
        """ In place add either adding number to every value without copy, or 
            directly extending internal frame buffer.
        """
        if isinstance(value, numbers.Real):
            self.data = np.add(self.data, value)
        else:
            try:
                self.data = np.hstack((self.data, value))
            except TypeError as e:
                return NotImplemented

        return self

    def __radd__(self, value):
        return self + value


    ########################
    # (&) Mix operator (&) #
    ########################
    def __and__(self, value):
        """ Mix two SoundBuffers or two 2d arrays with compatible dimensions

            >>> wt = Wavetable()
            >>> wt2 = Wavetable(list(range(20)))
            >>> wto = wt2 & wt
            >>> len(wto)
            20            
        """

        cdef double[:] out

        try:
            out = np.add(self.data, value[:len(self.data)])
            return Wavetable(out)
        except TypeError as e:
            return NotImplemented

    def __iand__(self, value):
        """ Mix in place two SoundBuffers or two 2d arrays with compatible dimensions
        """
        self.data = np.add(self.data, value[:len(self.data)])
        return self

    def __rand__(self, value):
        return self & value


    ##############
    # Truthiness #
    ##############
    def __bool__(self):
        return bool(len(self))

    def __getitem__(self, position):
        if isinstance(position, int):
            return self.data[position]
        return Wavetable(self.data[position])

    def __len__(self):
        return 0 if self.data is None else len(self.data)


    ###################################
    # (*) Multiplication operator (*) #
    ###################################
    def __mul__(self, value):
        cdef int length = len(self.data)
        cdef double[:] out = np.zeros(length)

        if isinstance(value, numbers.Real):
            out = np.multiply(self.data, value)

        elif isinstance(value, Wavetable):
            out = np.multiply(self.data, value.data)

        elif isinstance(value, SoundBuffer):
            out = np.multiply(self.data, value.frames)

        elif isinstance(value, list):
            out = np.multiply(self.data, np.array(value))

        else:
            try:
                out = np.multiply(self.data, np.array(value))
            except TypeError:
                return NotImplemented

        return Wavetable(out)

    def __imul__(self, value):
        """ Multiply a SoundBuffer in place by a number, iterable or SoundBuffer
        """
        if isinstance(value, numbers.Real):
            self.data = np.multiply(self.data, value)

        elif isinstance(value, Wavetable):
            self.data = np.multiply(self.data, value.data)

        elif isinstance(value, SoundBuffer):
            self.data = np.multiply(self.data, value.frames)

        elif isinstance(value, list):
            self.data = np.multiply(self.data, np.array(value))

        else:
            try:
                self.data = np.multiply(self.data, np.array(value))
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
            out = np.subtract(self.data, value)
        if isinstance(value, SoundBuffer):
            out = np.subtract(self.data, value.frames)
        if isinstance(value, Wavetable):
            out = np.subtract(self.data, value.data)
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
            self.data = np.subtract(self.data, value)

        if isinstance(value, SoundBuffer):
            self.data = np.subtract(self.data, value.frames)

        if isinstance(value, Wavetable):
            self.data = np.subtract(self.data, value.data)
        else:
            try:
                self.data = np.subtract(self.data, value)
            except TypeError as e:
                return NotImplemented

    def __rsub__(self, value):
        return self - value


    def __repr__(self):
        return 'Wavetable({})'.format(self.data)

    def clip(self, minval=-1, maxval=1):
        return Wavetable(np.clip(self.data, minval, maxval))
        
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
        wavetable = _window(window_type, length)

        if wavetable is None:
            raise TypeError('Invalid envelope type')

        return self * wavetable

    def max(self):
        return np.amax(self.data)
 
    def repeat(self, int reps=2):
        if reps <= 1:
            return Wavetable(self.data)

        return Wavetable(list(self.data) * reps)

    def reverse(self):
        self.data = np.flip(self.data, 0)

    def reversed(self):
        return Wavetable(np.flip(self.data, 0))

    def taper(self, double length):
        cdef int framelength = <int>(self.samplerate * length)
        return self * _adsr(len(self), framelength, 0, 1, framelength)


cdef tuple _parse_polyseg(str score, int length, int wtlength):
    """ score = '1,tri .5,sine,0-.5 sine!.25 tri,.1-.2!.5'
    """    
    cdef list segments = [] 
    cdef str segment
    cdef str param
    cdef int count = 0
    cdef int segment_wtype
    cdef double segment_slew = 0
    cdef double segment_start = 0
    cdef double segment_end = 1
    cdef int segment_length = wtlength
    cdef int total_segment_length = 0

    for segment in score.split(' '):
        segment_start = 0
        segment_end = 1
        segment_slew = 0
        segment_wtype = SINE
        segment_length = wtlength

        print(segment)
        match = SEGMENT_RE.match(segment)
        print(match)
        
        length = match.group('length')
        if length is not None:
            segment_length = <int>(wtlength * float(length))

        wtype = match.group('wtype')
        if wtype is not None:
            segment_wtype = wtypentf(wtype)

        start = match.group('start')
        if start is not None:
            segment_start = float(start)

        end = match.group('end')
        if end is not None:
            segment_end = float(end)

        segments += (segment_length, segment_wtype, segment_start, segment_end)
        total_segment_length += segment_length

    print(segments)

    return segments, total_segment_length

cpdef double[:] polyseg(list segments, int length):
    """ Calculate total output segment length in frames & alloc outbuf

        loop thru segments:
            - generate segment
            - write segment into outbuf

        for each segment:
            - calc the true length of the individual segment based on segment length
            - scale the size of the tmp segment to match remainder after start/end filtering
            - produce a tmp array from the waveform type (using normal wavetable or window generator)
            - copy into outbuf

        segment crossfading?

    """
    for segment_length, segment_wtype, segment_start, segment_end in segments:
        pass

    cdef double[:] out = np.zeros(length, dtype='d')

    return out


cpdef double[:] randline(int numpoints, double lowvalue=0, double highvalue=1, int wtsize=4096):
    points = np.array([ random.triangular(lowvalue, highvalue) for _ in range(numpoints) ], dtype='d')
    return interpolation._linear(points, wtsize)

cdef double[:] _window(int window_type, int length):
    cdef double[:] wt

    if window_type == RND:
        window_type = ALL_WINDOWS[rand() % LEN_WINDOWS]
        wt = _window(window_type, length)

    elif window_type == SINE:
        wt = np.sin(np.linspace(0, np.pi, length, dtype='d'))

    elif window_type == TRI:
        wt = np.abs(np.abs(np.linspace(0, 2, length, dtype='d') - 1) - 1)

    elif window_type == SAW:
        wt = np.linspace(0, 1, length, dtype='d')

    elif window_type == RSAW:
        wt = np.linspace(1, 0, length, dtype='d')

    elif window_type == HANN:
        wt = np.hanning(length)

    elif window_type == HAMM:
        wt = np.hamming(length)

    elif window_type == BART:
        wt = np.bartlett(length)

    elif window_type == BLACK:
        wt = np.blackman(length)

    elif window_type == KAISER:
        wt = np.kaiser(length, 0)

    else:
        wt = window(SINE, length)

    return wt

cdef double[:] _adsr(int framelength, int attack, int decay, double sustain, int release):
    cdef int decay_breakpoint = decay + attack
    cdef int sustain_breakpoint = framelength - release
    cdef int decay_framelength = decay_breakpoint - attack
    cdef int release_framelength = framelength - sustain_breakpoint
    cdef double[:] out = np.zeros(framelength, dtype='d')

    for i in range(framelength):
        if i <= attack and attack > 0:
            out[i] = i / <double>attack

        elif i <= decay_breakpoint and decay_breakpoint > 0:
            out[i] = (1 - ((i - attack) / <double>decay_framelength)) * (1 - sustain) + sustain
    
        elif i <= sustain_breakpoint:
            out[i] = sustain

        else:
            out[i] = (1 - ((i - sustain_breakpoint) / <double>release_framelength)) * sustain

    return out

cpdef double[:] adsr(int length, int attack, int decay, double sustain, int release):
    return _adsr(length, attack, decay, sustain, release)

def window(int window_type, int length, double[:] data=None):
    if data is not None:
        return interpolation._linear(data, length)

    return _window(window_type, length)

cdef double[:] _wavetable(int wavetable_type, int length):
    cdef double[:] wt

    if wavetable_type == RND:
        wavetable_type = ALL_WAVETABLES[rand() % LEN_WAVETABLES]
        wt = _wavetable(wavetable_type, length)

    elif wavetable_type == SINE:
        wt = np.sin(np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False))

    elif wavetable_type == COS:
        wt = np.cos(np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False))

    elif wavetable_type == TRI:
        tmp = np.abs(np.linspace(-1, 1, length, dtype='d', endpoint=False))
        tmp = np.abs(tmp)
        wt = (tmp - tmp.mean()) * 2

    elif wavetable_type == SAW:
        wt = np.linspace(-1, 1, length, dtype='d', endpoint=False)

    elif wavetable_type == RSAW:
        wt = np.linspace(1, -1, length, dtype='d', endpoint=False)

    elif wavetable_type == SQUARE:
        tmp = np.zeros(length, dtype='d')
        duty = int(length * SQUARE_DUTY)
        tmp[:duty] = 1
        tmp[duty:] = -1
        wt = tmp

    else:
        wt = _wavetable(SINE, length)

    return wt

cpdef double[:] wavetable(int wavetable_type, int length, double[:] data=None):
    if data is not None:
        return interpolation._linear(data, length)

    return _wavetable(wavetable_type, length)

cpdef double[:] fromfile(unicode filename, int length):
    wt, _ = soundfile.read(filename, dtype='d')
    if len(wt) == length:
        return wt

    return interpolation._linear(wt, length)


