""" Some helpers for building and transforming onset lists
"""

from . import wavetables

HIT_SYMBOLS = set((1, '1', 'X', 'x', True))
REST_SYMBOLS = set(('0', '.', ' ', '-'))

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

def topattern(pat, reverse=False):
    pat = [ 0 if tick in REST_SYMBOLS or not tick else 1 for tick in pat ]
    if reverse:
        pat = [ p for p in reversed(pat) ]
    return pat

def eu(length, numbeats, offset=0, reps=None, reverse=False):
    """ A euclidian pattern generator

        Length 12, numbeats 3

        >>> rhythm.eu(12, 3)
        [1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0]

        Length 6, numbeats 3

        >>> rhythm.eu(6, 3)
        [1, 0, 1, 0, 1, 0]

        Length 6, numbeats 3, offset 1
        >>> rhythm.eu(6, 3, 1)
        [0, 1, 0, 1, 0, 1]

        Length 6, numbeats 3, offset 1, reps 2
        >>> rhythm.eu(6, 3, 1)
        [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1]

        Length 6, numbeats 3, offset 1, reps 2, reverse True
        >>> rhythm.eu(6, 3, 1)
        [1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0]

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
        pattern = reversed(pattern)

    return pattern

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
    return pattern[-offset % len(pattern):] + pattern[:-offset % len(pattern)]

def repeat(onsets, reps):
    """ Repeat a sequence of onsets a given number of times
    """
    out = []
    total = sum(pattern)
    for rep in range(reps):
        out += [ onset + (rep * total) for onset in onsets ]

    return out

