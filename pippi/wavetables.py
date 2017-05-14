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

def interp(data, length, freq=None, phase=0, channels=2, samplerate=44100):
    out = np.zeros((length, channels))

    if freq is None:
        freq = samplerate / length

    readindex = 0
    inputlength = len(data)

    for i in range(length):
        readindex = int(phase) % inputlength

        val = data[readindex]

        try:
            nextval = data[readindex + 1]
        except IndexError:
            nextval = data[0]

        frac = phase - int(phase)

        val = (1.0 - frac) * val + frac * nextval

        for channel in range(channels):
            out[i][channel] = val

        phase += freq * inputlength * (1.0 / samplerate)

    return out, phase
