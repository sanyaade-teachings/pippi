""" Some helpers for building and transforming onset lists
"""

from . import wavetables

HIT_SYMBOLS = set((1, '1', 'X', 'x', True))

def pattern(numbeats, div=1, offset=0, reps=None, reverse=False):
    """ TODO: Pattern creation helper
    """
    pass

def eu(numbeats, length, offset=0, reps=None, reverse=False):
    """ TODO: euclidean pattern generation
    """
    pass

def onsets(pattern, beatlength=4410, offset=0, reps=None, reverse=False, playhead=0):
    """ TODO: convert ascii and last patterns into onset lists
    """
    out = []
    for beat in pattern:
        if beat in HIT_SYMBOLS:
            out += [ playhead ]

        playhead += beatlength

    if reverse:
        out = reversed(out)

    return out

def curve(numbeats=16, wintype=None, length=44100, reverse=False):
    """ Bouncy balls
    """
    wintype = wintype or 'random'

    if reverse:
        win = wavetables.window(wintype, numbeats * 2)[numbeats:]
    else:
        win = wavetables.window(wintype, numbeats * 2)[:numbeats]

    return [ int(onset * length) for onset in win ]


def rotate(pattern, offset=0):
    """ Rotate a pattern list by a given offset
    """
    return pattern[offset:] + pattern[:offset]

def scale(onsets, factor):
    """ Scale a list of onsets by a given factor
    """
    return [ int(onset * factor) for onset in onsets ]

def repeat(onsets, reps):
    """ Repeat a sequence of onsets a given number of times
    """
    out = []
    total = sum(pattern)
    for rep in range(reps):
        out += [ onset + (rep * total) for onset in onsets ]

    return out

