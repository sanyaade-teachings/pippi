import collections
import random
import numpy as np
from . import interpolation

cdef inline set SINEWAVE_NAMES  = set(('sin', 'sine', 'sinewave'))
cdef inline set COSINE_NAMES  = set(('cos', 'cosine'))
cdef inline set TRIANGLE_NAMES  = set(('tri', 'triangle'))
cdef inline set SAWTOOTH_NAMES  = set(('saw', 'sawtooth', 'ramp', 'line', 'lin'))
cdef inline set RSAWTOOTH_NAMES = set(('isaw', 'rsaw', 'isawtooth', 'rsawtooth', 'reversesaw', 'phasor'))
cdef inline set HANNING_NAMES = set(('hanning', 'hann', 'han'))
cdef inline set HAMMING_NAMES = set(('hamming', 'hamm', 'ham'))
cdef inline set BLACKMAN_NAMES = set(('blackman', 'black', 'bla'))
cdef inline set BARTLETT_NAMES = set(('bartlett', 'bar'))
cdef inline set KAISER_NAMES = set(('kaiser', 'kai'))
cdef inline set SQUARE_NAMES = set(('square', 'sq'))

cdef inline set ALL_WINDOWS = SINEWAVE_NAMES | TRIANGLE_NAMES | \
              SAWTOOTH_NAMES | RSAWTOOTH_NAMES | \
              HANNING_NAMES | HAMMING_NAMES | \
              BLACKMAN_NAMES | BARTLETT_NAMES | \
              KAISER_NAMES

cdef inline set ALL_WAVETABLES = SINEWAVE_NAMES | COSINE_NAMES | \
                 TRIANGLE_NAMES | SAWTOOTH_NAMES | \
                 RSAWTOOTH_NAMES | SQUARE_NAMES

def window(unicode window_type, int length, double[:] data=None):
    if data is not None:
        return interpolation.linear(data, length)

    cdef double[:] wt = None

    if window_type == u'random':
        window_type = random.choice(list(ALL_WINDOWS))

    if window_type in SINEWAVE_NAMES:
        wt = np.sin(np.linspace(0, np.pi, length, dtype='d'))

    if window_type in TRIANGLE_NAMES:
        wt = np.abs(np.abs(np.linspace(0, 2, length, dtype='d') - 1) - 1)

    if window_type in SAWTOOTH_NAMES:
        wt = np.linspace(0, 1, length, dtype='d')

    if window_type in RSAWTOOTH_NAMES:
        wt = np.linspace(1, 0, length, dtype='d')

    if window_type in HANNING_NAMES:
        wt = np.hanning(length)

    if window_type in HAMMING_NAMES:
        wt = np.hamming(length)

    if window_type in BARTLETT_NAMES:
        wt = np.bartlett(length)

    if window_type in BLACKMAN_NAMES:
        wt = np.blackman(length)

    if window_type in KAISER_NAMES:
        wt = np.kaiser(length, 0)

    if wt is None:
        return window(u'sine', length)

    return wt


def wavetable(unicode wavetable_type, int length, double duty=0.5, double[:] data=None):
    if data is not None:
        return interpolation.linear(data, length)

    cdef double[:] wt = None

    if wavetable_type == u'random':
        wavetable_type = random.choice(list(ALL_WAVETABLES))

    if wavetable_type in SINEWAVE_NAMES:
        wt = np.sin(np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False))

    if wavetable_type in COSINE_NAMES:
        wt = np.cos(np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False))

    if wavetable_type in TRIANGLE_NAMES:
        tmp = np.abs(np.linspace(-1, 1, length, dtype='d', endpoint=False))
        tmp = np.abs(tmp)
        wt = (tmp - tmp.mean()) * 2

    if wavetable_type in SAWTOOTH_NAMES:
        wt = np.linspace(-1, 1, length, dtype='d', endpoint=False)

    if wavetable_type in RSAWTOOTH_NAMES:
        wt = np.linspace(1, -1, length, dtype='d', endpoint=False)

    if wavetable_type in SQUARE_NAMES:
        tmp = np.zeros(length)
        duty = int(length * duty)
        tmp[:duty] = 1
        tmp[duty:] = -1
        wt = tmp

    if wt is None:
        return wavetable(u'sine', length)

    return wt


