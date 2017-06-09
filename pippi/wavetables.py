import collections
import random
import numpy as np
from . import interpolation


SINEWAVE_NAMES  = set(('sin', 'sine', 'sinewave'))
COSINE_NAMES  = set(('cos', 'cosine'))
TRIANGLE_NAMES  = set(('tri', 'triangle'))
SAWTOOTH_NAMES  = set(('saw', 'sawtooth', 'ramp', 'line', 'lin'))
RSAWTOOTH_NAMES = set(('isaw', 'rsaw', 'isawtooth', 'rsawtooth', 'reversesaw', 'phasor'))
HANNING_NAMES = set(('hanning', 'hann', 'han'))
HAMMING_NAMES = set(('hamming', 'hamm', 'ham'))
BLACKMAN_NAMES = set(('blackman', 'black', 'bla'))
BARTLETT_NAMES = set(('bartlett', 'bar'))
KAISER_NAMES = set(('kaiser', 'kai'))
SQUARE_NAMES = set(('square', 'sq'))

ALL_WINDOWS = SINEWAVE_NAMES | TRIANGLE_NAMES | \
              SAWTOOTH_NAMES | RSAWTOOTH_NAMES | \
              HANNING_NAMES | HAMMING_NAMES | \
              BLACKMAN_NAMES | BARTLETT_NAMES | \
              KAISER_NAMES

ALL_WAVETABLES = SINEWAVE_NAMES | COSINE_NAMES | \
                 TRIANGLE_NAMES | SAWTOOTH_NAMES | \
                 RSAWTOOTH_NAMES | SQUARE_NAMES

def window(window_type=None, length=None, data=None):
    if data is not None:
        return interpolation.linear(data, length)

    wt = None

    if window_type == 'random':
        window_type = random.choice(ALL_WINDOWS)

    if window_type in SINEWAVE_NAMES:
        wt = np.linspace(0, np.pi, length, dtype='d')
        wt = np.sin(wt) 

    if window_type in TRIANGLE_NAMES:
        wt = np.linspace(0, 2, length, dtype='d')
        wt = np.abs(np.abs(wt - 1) - 1)

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
        return window('sine', length)

    return wt


def wavetable(wavetable_type=None, length=None, duty=0.5, data=None):
    if data is not None:
        return interpolation.linear(data, length)

    wt = None

    if wavetable_type is None:
        wavetable_type = 'sine'
    elif wavetable_type == 'random':
        wavetable_type = random.choice(ALL_WAVETABLES)

    if wavetable_type in SINEWAVE_NAMES:
        wt = np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False)
        wt = np.sin(wt) 

    if wavetable_type in COSINE_NAMES:
        wt = np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False)
        wt = np.cos(wt) 

    if wavetable_type in TRIANGLE_NAMES:
        wt = np.linspace(-1, 1, length, dtype='d', endpoint=False)
        wt = np.abs(wt)
        wt = (wt - wt.mean()) * 2

    if wavetable_type in SAWTOOTH_NAMES:
        wt = np.linspace(-1, 1, length, dtype='d', endpoint=False)

    if wavetable_type in RSAWTOOTH_NAMES:
        wt = np.linspace(1, -1, length, dtype='d', endpoint=False)

    if wavetable_type in SQUARE_NAMES:
        wt = np.zeros(length)
        duty = int(length * duty)
        wt[:duty] = 1
        wt[duty:] = -1

    if wt is None:
        return wavetable('sine', length)

    return wt


