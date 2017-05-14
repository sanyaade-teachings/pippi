import numpy as np

SINEWAVE_NAMES  = ('sin', 'sine', 'sinewave')
COSINE_NAMES  = ('cos', 'cosine')
TRIANGLE_NAMES  = ('tri', 'triangle')
SAWTOOTH_NAMES  = ('saw', 'sawtooth', 'ramp', 'line')
RSAWTOOTH_NAMES = ('isaw', 'rsaw', 'isawtooth', 'rsawtooth', 'reversesaw', 'phasor')

def window(window_type=None, length=None):
    if window_type is None:
        window_type = 'sine'

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

    return wavetable


def wavetable(wavetable_type=None, length=None):
    if wavetable_type is None:
        wavetable_type = 'sine'

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

    return wavetable


