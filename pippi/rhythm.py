""" Some helpers for building and transforming onset lists
"""

from .wavetables import window

def grid(numbeats, beatlength, offset=0, stride=None, reps=None):
    """ Create a grid of onset times, to use as dubbing positions
        into a buffer.

        TODO: document options
    """
    onsets = [ beatlength * i for i in range(numbeats) ]
    onsets = onsets[offset:] + onsets[:offset]
    if stride is not None:
        subset = onsets[0:-1:stride]
        onsets = []
        for i in range(numbeats):
            onsets += [ subset[i % len(subset)] ]

    if reps is not None:
        out = []
        for rep in range(reps):
            for onset in onsets:
                out += [ onset * (rep + 1) ]
        onsets = out

    return onsets

def curve(numbeats=16, wintype=None, div=None, reverse=False):
    """ Bouncy balls
        Div sets the min division size
    """
    wintype = wintype or 'random'
    div = div or (44100//16)

    if reverse:
        win = window(wintype, numbeats * 2)[numbeats:]
    else:
        win = window(wintype, numbeats * 2)[:numbeats]

    assert len(win) == numbeats

    return [ int(div * w) for w in win ]

def rotate(pattern, offset=0):
    """ Rotate a list of onsets by a given offset
    """
    return pattern[offset:] + pattern[:offset]

def scale(pattern, factor):
    """ Scale a list of onsets by a given factor
    """
    return [ int(p * factor) for p in pattern ]

def repeat(pattern, reps):
    """ Repeat a sequence of onsets a given number of times
    """
    out = []
    for rep in range(reps):
        for p in pattern:
            out += [ p * (rep + 1) ]

    return out

