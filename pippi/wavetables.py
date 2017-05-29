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

    if window_type is None:
        window_type = 'sine'
    elif window_type == 'random':
        window_type = random.choice(ALL_WINDOWS)

    if window_type in SINEWAVE_NAMES:
        wavetable = np.linspace(0, np.pi, length, dtype='d')
        wavetable = np.sin(wavetable) 

    if window_type in TRIANGLE_NAMES:
        wavetable = np.linspace(0, 2, length, dtype='d')
        wavetable = np.abs(np.abs(wavetable - 1) - 1)

    if window_type in SAWTOOTH_NAMES:
        wavetable = np.linspace(0, 1, length, dtype='d')

    if window_type in RSAWTOOTH_NAMES:
        wavetable = np.linspace(1, 0, length, dtype='d')

    if window_type in HANNING_NAMES:
        wavetable = np.hanning(length)

    if window_type in HAMMING_NAMES:
        wavetable = np.hamming(length)

    if window_type in BARTLETT_NAMES:
        wavetable = np.bartlett(length)

    if window_type in BLACKMAN_NAMES:
        wavetable = np.blackman(length)

    if window_type in KAISER_NAMES:
        wavetable = np.kaiser(length, 0)

    return wavetable


def wavetable(wavetable_type=None, length=None, duty=0.5, data=None):
    if data is not None:
        return interpolation.linear(data, length)

    if wavetable_type is None:
        wavetable_type = 'sine'
    elif wavetable_type == 'random':
        wavetable_type = random.choice(ALL_WAVETABLES)

    if wavetable_type in SINEWAVE_NAMES:
        wavetable = np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False)
        wavetable = np.sin(wavetable) 

    if wavetable_type in COSINE_NAMES:
        wavetable = np.linspace(-np.pi, np.pi, length, dtype='d', endpoint=False)
        wavetable = np.cos(wavetable) 

    if wavetable_type in TRIANGLE_NAMES:
        wavetable = np.linspace(-1, 1, length, dtype='d', endpoint=False)
        wavetable = np.abs(wavetable)
        wavetable = (wavetable - wavetable.mean()) * 2

    if wavetable_type in SAWTOOTH_NAMES:
        wavetable = np.linspace(-1, 1, length, dtype='d', endpoint=False)

    if wavetable_type in RSAWTOOTH_NAMES:
        wavetable = np.linspace(1, -1, length, dtype='d', endpoint=False)

    if wavetable_type in SQUARE_NAMES:
        wavetable = np.zeros(length)
        duty = int(length * duty)
        wavetable[:duty] = 1
        wavetable[duty:] = -1

    return wavetable


