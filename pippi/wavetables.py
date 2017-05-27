import collections
import random
import numpy as np

SINEWAVE_NAMES  = ('sin', 'sine', 'sinewave')
COSINE_NAMES  = ('cos', 'cosine')
TRIANGLE_NAMES  = ('tri', 'triangle')
SAWTOOTH_NAMES  = ('saw', 'sawtooth', 'ramp', 'line', 'lin')
RSAWTOOTH_NAMES = ('isaw', 'rsaw', 'isawtooth', 'rsawtooth', 'reversesaw', 'phasor')
HANNING_NAMES = ('hanning', 'hann', 'han')
HAMMING_NAMES = ('hamming', 'hamm', 'ham')
BLACKMAN_NAMES = ('blackman', 'black', 'bla')
BARTLETT_NAMES = ('bartlett', 'bar')
KAISER_NAMES = ('kaiser', 'kai')
SQUARE_NAMES = ('square', 'sq')

ALL_WINDOWS = (
    SINEWAVE_NAMES[0],
    TRIANGLE_NAMES[0], 
    SAWTOOTH_NAMES[0], 
    RSAWTOOTH_NAMES[0], 
    HANNING_NAMES[0], 
    HAMMING_NAMES[0], 
    BLACKMAN_NAMES[0], 
    BARTLETT_NAMES[0], 
    KAISER_NAMES[0], 
)

ALL_WAVETABLES = (
    SINEWAVE_NAMES[0],
    COSINE_NAMES[0],
    TRIANGLE_NAMES[0], 
    SAWTOOTH_NAMES[0], 
    RSAWTOOTH_NAMES[0], 
    SQUARE_NAMES[0], 
)

def window(window_type=None, length=None, data=None):
    if data is not None:
        return interp(data, length)

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
        return interp(data, length)

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

def interp(data, length):
    out = []

    readindex = 0
    inputlength = len(data)
    phase = 0

    for i in range(length):
        readindex = int(phase) % inputlength

        val = data[readindex]

        try:
            nextval = data[readindex + 1]
        except IndexError:
            nextval = data[0]

        frac = phase - int(phase)

        val = (1.0 - frac) * val + frac * nextval

        out += [ val ]

        phase += inputlength * (1.0 / length)

    return out
