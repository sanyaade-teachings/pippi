import numpy as np

SINEWAVE_NAMES  = ('sin', 'sine', 'sinewave')
TRIANGLE_NAMES  = ('tri', 'triangle')
SAWTOOTH_NAMES  = ('saw', 'sawtooth', 'ramp', 'line')
RSAWTOOTH_NAMES = ('isaw', 'rsaw', 'isawtooth', 'rsawtooth', 'reversesaw', 'phasor')

def window(window_type=None, length=None, values=None):
    if window_type is None:
        window_type = 'sine'

    if values is not None:
        # FIXME interpolate to len(self)
        wavetable = values

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


