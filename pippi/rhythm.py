""" Some helpers for building and transforming onset lists
"""

from . import wavetables
from . import interpolation

REST_SYMBOLS = set(('0', '.', ' ', '-', 0, False))

def pattern(numbeats, div=1, offset=0, reps=None, reverse=False):
    """ Pattern creation helper
    """
    pat = [ 1 if tick == 0 else 0 for tick in range(div) ]
    pat = [ pat[i % len(pat)] for i in range(numbeats) ]

    if offset > 0:
        pat = [ 0 for _ in range(offset) ] + pat[:-offset]

    if reps is not None:
        pat *= reps

    if reverse:
        pat = [ p for p in reversed(pat) ]

    return pat

def topattern(pattern, reverse=False):
    pattern = [ 0 if tick in REST_SYMBOLS or not tick else 1 for tick in pattern ]
    if reverse:
        pattern = [ p for p in reversed(pattern) ]
    return pattern

def onsets(pattern, beat=4410, length=None, start=0):
    length = length or len(pattern)
    grid = [ beat * i + start for i in range(length) ]
    pattern = [ pattern[i % len(pattern)] for i in range(length) ]

    out = []

    for i, tick in enumerate(pattern):
        if tick not in REST_SYMBOLS:
            pos = grid[i % len(grid)]
            out += [ pos ]
        
    return out

def eu(length, numbeats, offset=0, reps=None, reverse=False):
    """ A euclidian pattern generator

        Length 6, numbeats 3
        >>> rhythm.eu(6, 3)
        [1, 0, 1, 0, 1, 0]

        Length 6, numbeats 3, offset 1
        >>> rhythm.eu(6, 3, 1)
        [0, 1, 0, 1, 0, 1]
    """
    pulses = [ 1 for pulse in range(numbeats) ]
    pauses = [ 0 for pause in range(length - numbeats) ]

    position = 0
    while len(pauses) > 0:
        try:
            index = pulses.index(1, position)
            pulses.insert(index + 1, pauses.pop(0))
            position = index + 1
        except ValueError:
            position = 0

    pattern = rotate(pulses, offset+len(pulses))

    if reps:
        pattern *= reps

    if reverse:
        pattern = [ p for p in reversed(pattern) ]

    return pattern

def swing(onsets, amount, beat, mpc=True):
    """ Add MPC-style swing to a list of onsets.
        Amount is a value between 0 and 1, which 
        maps to a swing amount between 0% and 75%.
        Every odd onset will be delayed by
            
            beat length * swing amount

        This will only really work like MPC swing 
        when there are a run of notes, as rests 
        are not represented in onset lists (and 
        MPC swing just delays the upbeats).
    """
    if amount <= 0:
        return onsets

    delay_amount = int(beat * amount * 0.75)
    for i, onset in enumerate(onsets):
        if i % 2 == 1:
            onsets[i] += delay_amount

    return onsets

def curve(numbeats=16, wintype=None, length=44100, reverse=False, wavetable=None):
    """ Bouncy balls
    """
    wintype = wintype or 'random'

    if wavetable is None:
        win = wavetables.window(wintype, numbeats * 2)
    else:
        win = interpolation.linear(wavetable, numbeats)

    if reverse:
        win = win[numbeats:]
    else:
        win = win[:numbeats]

    return [ int(onset * length) for onset in win ]

def rotate(pattern, offset=0):
    """ Rotate a pattern list by a given offset
    """
    return pattern[-offset % len(pattern):] + pattern[:-offset % len(pattern)]

def repeat(onsets, reps):
    """ Repeat a sequence of onsets a given number of times
    """
    out = []
    total = sum(pattern)
    for rep in range(reps):
        out += [ onset + (rep * total) for onset in onsets ]

    return out

