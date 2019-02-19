#cython: language_level=3

import collections
import numbers

import soundfile
cimport numpy as np
import numpy as np
import re

from cpython.array cimport array, clone

from libc.stdlib cimport malloc, realloc, calloc, free
from libc cimport math

from pippi cimport interpolation, rand, graph
from pippi.soundbuffer cimport SoundBuffer

cdef int SINE = 0
cdef int SINEIN = 17
cdef int SINEOUT = 18
cdef int COS = 1
cdef int TRI = 2
cdef int SAW = 3
cdef int RSAW = 4
cdef int HANN = 5
cdef int HANNIN = 21
cdef int HANNOUT = 22
cdef int HAMM = 6
cdef int BLACK = 7
cdef int BLACKMAN = 7
cdef int BART = 8
cdef int BARTLETT = 8
cdef int KAISER = 9
cdef int SQUARE = 10
cdef int RND = 11
cdef int LINE = SAW
cdef int PHASOR = SAW
cdef int SINC = 23

cdef int LINEAR = 12
cdef int TRUNC = 13
cdef int HERMITE = 14
cdef int CONSTANT = 15
cdef int GOGINS = 16

cdef int LEN_WINDOWS = 14
cdef int* ALL_WINDOWS = [
            SINE, 
            SINEIN, 
            SINEOUT, 
            COS,
            TRI, 
            SAW,
            RSAW,
            HANN,
            HANNIN,
            HANNOUT,
            HAMM,
            BLACK,
            BART,
            KAISER
        ]

cdef int LEN_WAVETABLES = 6
cdef int* ALL_WAVETABLES = [
            SINE, 
            COS,
            TRI,
            SAW,
            RSAW,
            SQUARE
        ]

cdef double SQUARE_DUTY = 0.5

SEGMENT_RE = re.compile('(?P<length>0?\.?\d+)?,?(?P<wtype>\w+),?(?P<start>0?\.?\d+)?-?(?P<end>0?\.?\d+)?')

cdef int to_flag(str value):
    cdef dict flags = {
        'sine': SINE, 
        'sinein': SINEIN, 
        'sineout': SINEOUT, 
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
        'line': LINE, 
        'hannin': HANNIN, 
        'hannout': HANNOUT, 
        'square': SQUARE, 
        'linear': LINEAR, 
        'trunc': TRUNC, 
        'hermite': HERMITE, 
        'constant': CONSTANT, 
        'gogins': GOGINS, 
        'sinc': SINC,
    }

    return flags[value]

cdef class Wavetable:
    def __cinit__(self, object values, 
            object lowvalue=None, 
            object highvalue=None,
            object wtsize=None, 
            bint window=False, 
            bint pad=False):
        cdef bint scaled = False
        cdef bint resized = False

        if window:
            self.data = to_window(values)
        else:
            self.data = to_wavetable(values)

        if lowvalue is None:
            self.lowvalue = np.min(self.data)
        else:
            scaled = True
            self.lowvalue = <double>lowvalue

        if highvalue is None:
            self.highvalue = np.max(self.data)
        else:
            scaled = True
            self.highvalue = <double>highvalue

        if scaled:
            self.data = np.interp(self.data, (np.min(self.data), np.max(self.data)), (self.lowvalue, self.highvalue))

        if wtsize is not None and len(self.data) != wtsize:
            self.length = wtsize
            self.data = interpolation._linear(self.data, self.length)
        else:
            self.length = len(self.data)

        if pad:
            self.pad()

    #############################################
    # (+) Addition & concatenation operator (+) #
    #############################################
    def __add__(self, value):
        cdef double[:] out = np.zeros(self.length)

        if isinstance(value, numbers.Real):
            out = np.add(self.data, value)
        elif isinstance(value, Wavetable):
            out = np.add(self.data, value.data)
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
        cdef double[:] out

        try:
            out = np.add(self.data, value[:len(self.data)])
            return Wavetable(out)
        except TypeError as e:
            return NotImplemented

    def __iand__(self, value):
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

        elif isinstance(value, list):
            out = np.multiply(self.data, np.array(value))

        else:
            try:
                out = np.multiply(self.data, np.array(value))
            except TypeError:
                return NotImplemented

        return Wavetable(out)

    def __imul__(self, value):
        if isinstance(value, numbers.Real):
            self.data = np.multiply(self.data, value)

        elif isinstance(value, Wavetable):
            self.data = np.multiply(self.data, value.data)

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
        cdef double[:,:] out

        if isinstance(value, numbers.Real):
            out = np.subtract(self.data, value)

        if isinstance(value, Wavetable):
            out = np.subtract(self.data, value.data)
        else:
            try:
                out = np.subtract(self.frames, value[:,None])
            except TypeError as e:
                return NotImplemented

        return Wavetable(out)

    def __isub__(self, value):
        if isinstance(value, numbers.Real):
            self.data = np.subtract(self.data, value)

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

    def drink(self, width=0.1, minval=None, maxval=None, indexes=None, wrap=False):
        if minval is None:
            minval = np.min(self.data)

        if maxval is None:
            maxval = np.max(self.data)

        if indexes is None:
            indexes = list(range(len(self.data)))

        for i in indexes:
            self.data[i] = max(minval, min(self.data[i] + rand.rand(-width, width), maxval))

        if wrap:
            self.data[len(self.data)-1] = self.data[0]

    def env(self, int window_type=-1):
        cdef int length = len(self)
        cdef double[:] wavetable = None
        wavetable = _window(window_type, length)

        if wavetable is None:
            raise TypeError('Invalid envelope type')

        return self * wavetable

    def graph(Wavetable self,
        str filename=None, 
        int width=400, 
        int height=300, 
        double offset=1, 
        double mult=0.5, 
        int stroke=3, 
        int upsample_mult=5, 
        bint show_axis=True):
        graph.write(self, filename, width, height, offset, mult, stroke, upsample_mult, show_axis)

    def max(self):
        return np.amax(self.data)

    def pad(self, numzeros=1):
        self.data = np.pad(self.data, (numzeros, numzeros), 'constant', constant_values=(0,0))
 
    def repeat(self, int reps=2):
        if reps <= 1:
            return self

        return Wavetable(list(self.data) * reps, wtsize=len(self.data) * reps)

    def reverse(self):
        self.data = np.flip(self.data, 0)

    def reversed(self):
        return Wavetable(np.flip(self.data, 0))

    def taper(self, int length):
        return self * _adsr(len(self), length, 0, 1, length)

    def interp(self, pos, method=LINEAR):
        if method == LINEAR:
            return interpolation._linear_point(self.data, pos)
        else:
            return interpolation._trunc_point(self.data, pos)


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

        match = SEGMENT_RE.match(segment)
        
        length = match.group('length')
        if length is not None:
            segment_length = <int>(wtlength * float(length))

        wtype = match.group('wtype')
        if wtype is not None:
            segment_wtype = to_flag(wtype)

        start = match.group('start')
        if start is not None:
            segment_start = float(start)

        end = match.group('end')
        if end is not None:
            segment_end = float(end)

        segments += (segment_length, segment_wtype, segment_start, segment_end)
        total_segment_length += segment_length

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


cpdef Wavetable _randline(int numpoints, double lowvalue=0, double highvalue=1, int wtsize=4096):
    cdef double[:] points = np.array([ rand.rand(lowvalue, highvalue) for _ in range(numpoints) ], dtype='d')
    return Wavetable(points, wtsize=wtsize)

cdef double[:] _window(int window_type, int length):
    cdef double[:] wt

    if window_type == RND:
        window_type = ALL_WINDOWS[rand.randint(0, LEN_WINDOWS-1)]
        wt = _window(window_type, length)

    elif window_type == SINE:
        wt = np.sin(np.linspace(0, np.pi, length, dtype='d'))

    elif window_type == SINEIN:
        wt = np.sin(np.linspace(0, np.pi/2, length, dtype='d'))

    elif window_type == SINEOUT:
        wt = np.sin(np.linspace(np.pi/2, np.pi, length, dtype='d'))

    elif window_type == COS:
        wt = (np.cos(np.linspace(0, np.pi*2, length, dtype='d')) + 1) * 0.5

    elif window_type == TRI:
        wt = np.bartlett(length)

    elif window_type == SAW:
        wt = np.linspace(0, 1, length, dtype='d')

    elif window_type == RSAW:
        wt = np.linspace(1, 0, length, dtype='d')

    elif window_type == HANN:
        wt = np.hanning(length)

    elif window_type == HANNIN:
        wt = np.hanning(length * 2)[:length]

    elif window_type == HANNOUT:
        wt = np.hanning(length * 2)[length:]

    elif window_type == HAMM:
        wt = np.hamming(length)

    elif window_type == BART:
        wt = np.bartlett(length)

    elif window_type == BLACK:
        wt = np.blackman(length)

    elif window_type == KAISER:
        wt = np.kaiser(length, 14)
        
    elif window_type == SINC:
        wt = np.sinc(np.linspace(-15, 15, length, dtype='d'))

    else:
        wt = _window(SINE, length)

    return wt

cdef double[:] _adsr(int framelength, int attack, int decay, double sustain, int release):
    cdef int alen = attack + decay + release
    cdef double mult = 1
    if alen > framelength:
        mult = <double>framelength / <double>alen
        attack = <int>(mult * attack)
        decay = <int>(mult * decay)
        release = <int>(mult * release)

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

cdef double[:] _wavetable(int wavetable_type, int length):
    cdef double[:] wt

    if wavetable_type == RND:
        wavetable_type = ALL_WAVETABLES[rand.randint(0, LEN_WAVETABLES-1)]
        wt = _wavetable(wavetable_type, length)

    elif wavetable_type == SINE:
        wt = np.sin(np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False))

    elif wavetable_type == COS:
        wt = np.cos(np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False))

    elif wavetable_type == TRI:
        wt = np.bartlett(length+1)[0:length] * 2 - 1

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

cpdef double[:] to_window(object w, int wtsize=4096):
    cdef double[:] wt

    if w is None:
        return None

    if isinstance(w, str):
        wt = _window(to_flag(w), wtsize)

    elif isinstance(w, numbers.Real):
        wt = np.full(1, w, dtype='d')

    elif isinstance(w, Wavetable):
        wt = w.data

    elif isinstance(w, SoundBuffer):
        wt = np.ravel(np.array(w.remix(1).frames, dtype='d'))

    else:
        wt = interpolation._linear(array('d', w), wtsize)

    return wt

cpdef double[:] to_wavetable(object w, int wtsize=4096):
    cdef double[:] wt

    if w is None:
        return None

    if isinstance(w, str):
        wt = _wavetable(to_flag(w), wtsize)

    elif isinstance(w, numbers.Real):
        wt = np.full(1, w, dtype='d')

    elif isinstance(w, SoundBuffer):
        wt = np.ravel(np.array(w.remix(1).frames, dtype='d'))

    elif isinstance(w, Wavetable):
        wt = w.data

    else:
        wt = interpolation._linear(array('d', w), wtsize)

    return wt

cpdef list to_lfostack(list lfos, int wtsize=4096):
    return [ interpolation._linear(to_wavetable(wt, wtsize), wtsize) for wt in lfos ]

cdef double[:] _seesaw(double[:] wt, int length, double tip=0.5):
    cdef double[:] out = np.zeros(length, dtype='d')
    cdef int wtlength = len(wt)
    cdef int i = 0
    cdef double x = wtlength * tip
    cdef double phase_inc = (1.0 / length) * wtlength
    cdef double warp=0, m=0, b=0
    cdef double phase = 0
    print('')

    for i in range(length):
        m = 0.5 / x
        if(phase < m):
            warp = m * phase
        else:
            m = 0.5 / (1.0 - x)
            b = 1.0 - m
            warp = m * phase + b

        out[i] = interpolation._linear_point(wt, phase) 
        phase += phase_inc

        print('i %02d' % i, 'out %.6f' % out[i], 'm %.6f' % m, 'b %.6f' % b, 'warp %.6f %s' % (warp, wtlength), 'phase %.6f' % phase)

    return out

cpdef Wavetable seesaw(object wt, int length, double tip=0.5):
    print('WT', wt)
    cdef double[:] _wt = to_wavetable(wt)
    cdef double[:] out = _seesaw(_wt, length, tip)
    return Wavetable(out)
