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


def get_freq_from_chord_name(name, root=440, octave=3, ratios=JUST):
    # No current refs internally...
    index = get_chord_root_index(name)
    freq = ratios[index]
    freq = root * (freq[0] / freq[1]) * 2**octave

    return freq

def next_chord(name):
    # No current refs internally... used w/ PROGRESSIONS
    name = strip_chord(name)
    return random.choice(PROGRESSIONS[name])

def strip_chord(name):
    root = re.sub('[#b+/*^0-9]+', '', name)
    return root.lower()

def get_chord_root_index(name):
    root = CHORD_ROMANS[strip_chord(name)]

    if '#' in name:
        root += 1

    if 'b' in name:
        root -= 1

    return root % 12

def get_quality(name):
    quality = '-' if name.islower() else '^'
    quality = '*' if re.match(MATCH_ROMAN + '[*]', name) is not None else quality

    return quality

def get_extension(name):
    return re.sub(MATCH_ROMAN + '[/*+]?', '', name)

def get_intervals(name):
    quality = get_quality(name)
    extension = get_extension(name)

    chord = BASE_QUALITIES[quality]

    if extension != '':
        chord = chord + EXTENSIONS[extension]

    return chord


def chord(name, key=None, octave=3, ratios=None):
    if key is None:
        key = DEFAULT_KEY

    if ratios is None:
        ratios = DEFAULT_RATIOS

    key = ntf(key, octave, ratios)
    root = ratios[get_chord_root_index(name)]
    root = key * (root[0] / root[1])

    name = re.sub('[#b]+', '', name)
    chord = get_intervals(name)
    chord = [ get_ratio_from_interval(iv, ratios) for iv in chord ]
    chord = [ root * ratio for ratio in chord ]

    return chord

def chords(names, key=None, octave=3, ratios=JUST):
    if key is None:
        key = DEFAULT_KEY

    return [ chord(name, key, octave, ratios) for name in names ]


