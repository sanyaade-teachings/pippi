#cython: language_level=3

import re
import random # FIXME use internal choice
from pippi.old import DEFAULT_KEY, DEFAULT_RATIOS, JUST, ntf, get_ratio_from_interval


MATCH_ROMAN = '[ivIV]?[ivIV]?[iI]?'

CHORD_ROMANS = {
    'i': 0,
    'ii': 2,
    'iii': 4,
    'iv': 5,
    'v': 7,
    'vi': 9,
    'vii': 11,
}

BASE_QUALITIES = {
    '^': ['P1', 'M3', 'P5'],   # major
    '-': ['P1', 'm3', 'P5'],   # minor
    '*': ['P1', 'm3', 'TT'],   # diminished 
    '+': ['P1', 'M3', 'm6'],   # augmented
}

EXTENSIONS = {
    '7': ['m7'],               # dominant 7th
    '^7': ['M7'],              # major 7th
    '9': ['m7', 'M9'],         # dominant 9th
    '^9': ['M7', 'M9'],        # major 9th
    '11': ['m7', 'M9', 'P11'], # dominant 11th
    '^11': ['M7', 'M9', 'P11'],
    '69': ['M6', 'M9'],        # dominant 6/9
    '6': ['M6'],        
}

# Common root movements
PROGRESSIONS = {
    'I': ['iii', 'vi', 'ii', 'IV', 'V', 'vii*'],
    'i': ['VII', 'III', 'VI', 'ii*', 'iv', 'V', 'vii*'],
    'ii': ['V', 'vii*'],
    'iii': ['vi'],
    'III': ['VI'],
    'IV': ['V', 'vii*'],
    'iv': ['V', 'vii*'],
    'V': ['I', 'i'], # a pivot
    'v': ['I', 'i'], # a pivot
    'vi': ['ii', 'IV'],
    'VI': ['ii*', 'iv'],
    'vii*': ['I', 'i'], # a pivot
    'vii': ['I', 'i'], # a pivot
}


def next_chord(name):
    # No current refs internally... used w/ PROGRESSIONS
    name = strip_chord(name)
    return random.choice(PROGRESSIONS[name])

def strip_chord(name):
    root = re.sub('[#b+*^0-9]+', '', name)
    return root.lower()

def name_to_extension(name):
    return re.sub(MATCH_ROMAN + '[/*+]?', '', name)

def name_to_quality(name):
    quality = '-' if name.islower() else '^'
    return '*' if re.match(MATCH_ROMAN + '[*]', name) is not None else quality

def name_to_intervals(name):
    quality = name_to_quality(name)
    extension = name_to_extension(name) 

    intervals = BASE_QUALITIES[quality]

    if extension != '':
        intervals = intervals + EXTENSIONS[extension]

    return intervals

def name_to_index(name):
    # Chord name to index
    root = CHORD_ROMANS[strip_chord(name)]

    if '#' in name:
        root += 1

    if 'b' in name:
        root -= 1

    return root % 12

def chord(name, key=None, octave=3, ratios=None):
    if key is None:
        key = DEFAULT_KEY

    if ratios is None:
        ratios = DEFAULT_RATIOS

    key = ntf(key, octave, ratios)
    root = ratios[name_to_index(name)]
    root = key * (root[0] / root[1])

    name = re.sub('[#b]+', '', name)
    intervals = name_to_intervals(name)
    _ratios = [ get_ratio_from_interval(iv, ratios) for iv in intervals ]
    chord = [ root * ratio for ratio in _ratios ]

    return chord

def chords(names, key=None, octave=3, ratios=JUST):
    if key is None:
        key = DEFAULT_KEY

    return [ chord(name, key, octave, ratios) for name in names ]


