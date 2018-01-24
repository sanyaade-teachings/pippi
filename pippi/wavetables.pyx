import collections
import random

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

SEGMENT_RE = re.compile(r'((?P<length>\.?\d+),)?(?P<wtype>\w+)(,(?P<start>\.?\d+)-(?P<end>\.?\d+))?')

cdef int wtypentf(str wtype_name):
    return wtype_flags[wtype_name]

cpdef double[:] polyseg(str score, int length, int wtlength=4096):
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

        for count, param in enumerate(segment.split(',')):
            match = SEGMENT_RE.match(param)
            
            length = match.group('length')
            if length is not None:
                segment_length = <int>(wtlength * float(length))

            wtype = match.group('wtype')
            if wtype is not None:
                segment_wtype = wtypentf(param)

            start = match.group('start')
            if start is not None:
                segment_start = float(start)

            end = match.group('end')
            if end is not None:
                segment_end = float(end)

        segments += (segment_length, segment_wtype, segment_start, segment_end)
        total_segment_length += segment_length

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


