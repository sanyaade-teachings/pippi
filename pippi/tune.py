from pippi import dsp
import re

a0 = 27.5 
a4 = a0 * 2**4
default_key = 'c'
match_roman = '[ivIV]?[ivIV]?[iI]?'

intervals = {
    'P1': 0, 
    'm2': 1, 
    'M2': 2, 
    'm3': 3, 
    'M3': 4, 
    'P4': 5, 
    'TT': 6, 
    'P5': 7, 
    'm6': 8, 
    'M6': 9, 
    'm7': 10, 
    'M7': 11,
    'P8': 12,
    'm9': 13,
    'M9': 14,
    'm10': 15,
    'M10': 16,
    'P11': 17,
    'd12': 18, # TT + P8
    'P12': 19,
    'm13': 20,
    'M13': 21,
    'm14': 22,
    'M14': 23,
    'P15': 24
}

# what's the best way to handle inversions?
base_qualities = {
    '^': ['P1', 'M3', 'P5'],   # major
    '-': ['P1', 'm3', 'P5'],   # minor
    '*': ['P1', 'm3', 'TT'],   # diminished 
    '+': ['P1', 'M3', 'm6'],   # augmented
}

extensions = {
    '7': ['m7'],               # dominant 7th
    '^7': ['M7'],              # major 7th
    '9': ['m7', 'M9'],         # dominant 9th
    '^9': ['M7', 'M9'],        # major 9th
    '11': ['m7', 'M9', 'P11'], # dominant 11th
    '^11': ['M7', 'M9', 'P11'],
    '69': ['M6', 'M9'],        # dominant 6/9
    '6': ['M6'],        
}

chord_romans = {
    'i': 0,
    'ii': 2,
    'iii': 4,
    'iv': 5,
    'v': 7,
    'vi': 9,
    'vii': 11,
}

# Common root movements
progressions = {
    'I': ['iii', 'vi', 'ii', 'IV', 'V', 'vii*'],
    'i': ['VII', 'III', 'VI', 'ii*', 'iv', 'V', 'vii*'],
    'ii': ['V', 'vii*'],
    'iii': ['vi'],
    'III': ['VI'],
    'IV': ['V', 'vii*'],
    'iv': ['V', 'vii*'],
    'V': ['I', 'i'], # a pivot
    'vi': ['ii', 'IV'],
    'VI': ['ii*', 'iv'],
    'vii*': ['I', 'i'], # a pivot
}

def etmult(pos):
    return 2**(pos/12.0)

et = (
    (etmult(0), 1.0),
    (etmult(1), 1.0),
    (etmult(2), 1.0),
    (etmult(3), 1.0),
    (etmult(4), 1.0),
    (etmult(5), 1.0),
    (etmult(6), 1.0),
    (etmult(7), 1.0),
    (etmult(8), 1.0),
    (etmult(9), 1.0),
    (etmult(10), 1.0),
    (etmult(11), 1.0),
)

just = ( 
    (1.0, 1.0),     # P1
    (16.0, 15.0),   # m2
    (9.0, 8.0),     # M2
    (6.0, 5.0),     # m3
    (5.0, 4.0),     # M3
    (4.0, 3.0),     # P4
    (45.0, 32.0),   # TT
    (3.0, 2.0),     # P5
    (8.0, 5.0),     # m6
    (5.0, 3.0),     # M6
    (9.0, 5.0),     # m7
    (15.0, 8.0),    # M7
)

terry = (
    (1.0, 1.0),     # P1  C
    (16.0, 15.0),   # m2  Db
    (10.0, 9.0),    # M2  D
    (6.0, 5.0),     # m3  Eb
    (5.0, 4.0),     # M3  E
    (4.0, 3.0),     # P4  F
    (64.0, 45.0),   # TT  Gb
    (3.0, 2.0),     # P5  G
    (8.0, 5.0),     # m6  Ab
    (27.0, 16.0),   # M6  A 
    (16.0, 9.0),    # m7  Bb
    (15.0, 8.0),    # M7  B
)

young = (
    (1.0, 1.0),       # P1 0
    (567.0, 512.0),   # m2 1
    (9.0, 8.0),       # M2 2
    (147.0, 128.0),   # m3 3
    (21.0, 16.0),     # M3 4
    (1323.0, 1024.0), # P4 5
    (189.0, 128.0),   # TT 6
    (3.0, 2.0),       # P5 7
    (49.0, 32.0),     # m6 8
    (7.0, 4.0),       # M6 9
    (441.0, 256.0),   # m7 10
    (63.0, 32.0),     # M7 11
)

louis = (
    (1.0, 1.0),       # P1 1.0
    (1.0, 1.0),       # m2 1.0
    (9.0, 8.0),       # M2 1.125
    (9.0, 8.0),       # m3 1.125
    (5.0, 4.0),       # M3 1.25
    (5.0, 4.0),       # P4 1.25
    (3.0, 2.0),       # TT 1.5
    (3.0, 2.0),       # P5 1.5
    (8.0, 5.0),       # m6 1.6
    (7.0, 4.0),       # M6 1.75
    (9.0, 5.0),       # m7 1.8
    (9.0, 5.0),       # M7 1.8
)


## scale mappings
# standard
major = (0, 2, 4, 5, 7, 9, 11)
minor = (0, 2, 3, 5, 7, 8, 10)

# notated as semitone deviations from 
# a major scale for readability
scales = {
    'major': major,
    'minor': minor,
    'super_locrian': (0, 2-1, 4-1, 5-1, 7-1, 9-1, 11-1),
    'neapolitan_minor': (0, 2-1, 4-1, 5, 7, 9-1, 11),
    'neapolitan_major': (0, 2-1, 4-1, 5, 7, 9, 11),
    'oriental': (0, 2-1, 4, 5, 7-1, 9, 11-1),
    'double_harmonic': (0, 2-1, 4, 5, 7, 9-1, 11),
    'enigmatic': (0, 2-1, 4, 5+1, 7+1, 9+1, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
    'major': (0, 2, 4, 5, 7, 9, 11),
}

# Maps to chromatic ratio lists above
notes = { 
        'a': 0,
        'a#': 1,
        'bb': 1, 
        'b': 2,
        'c': 3, 
        'c#': 4, 
        'db': 4, 
        'd': 5, 
        'd#': 6, 
        'eb': 6, 
        'e': 7, 
        'f': 8, 
        'f#': 9, 
        'gb': 9, 
        'g': 10, 
        'g#': 11, 
        'ab': 11, 
        }


def nti(note):
    """ Note to index
            returns the index of enharmonic note names
            or False if not found
    """
    return notes.get(note, False)

def ntf(note, octave=None, ratios=None):
    """ Note to freq 
    """
    if re.match('[a-zA-Z]#?b?\d+', note) is not None:
        parsed = re.match('([a-zA-Z]#?b?)(\d+)', note)
        note = parsed.group(1)
        octave = int(parsed.group(2))

    if ratios is None:
        ratios = terry

    if octave is None:
        octave = 4

    note_index = nti(note)
    mult = ratios[note_index][0] / ratios[note_index][1]

    if note_index >= 3:
        octave -= 1

    return mult * a0 * 2.0**octave

def stf(index):
    degree = index % 24
    octave = index / 24

    return (2 ** (degree / 24.0)) * (a0 / 4.0) * (2.0 ** octave)

def mtf(midi_note):
    return 2**((midi_note - 69) / 12.0) * 440.0

def fts(freq):
    # Try to find closest eq temp freq to input
    # Generate entire range of possible eq temp freqs
    all_freq = [ stf(index) for index in range(2**8) ]

    count = 0
    cfreq = 0
    while freq > cfreq and count < 2**8:
        cfreq = all_freq[count]
        count = count + 1

    count = count - 1

    return count % 55

def nts(note, octave):
    octave = octave if octave >= -2 else -2
    octave = octave if octave <= 8 else 8 

    degree = notes[note] * 2

    degree = degree + ((octave + 2) * 24) 

    return degree

def getscale(degrees, root=261.63, ratios=None, scale=None):
    if ratios is None:
        ratios = terry

    if scale is None:
        scale = major

    multipliers = []

    for degree in degrees:
        base = scale[(degree - 1) % len(scale)]
        octave = degree / (len(scale) + 1)

        multiplier = ratios[base][0] / ratios[base][1]
        multiplier = multiplier * 2**octave

        multipliers += [ multiplier ]

    return [ root * m for m in multipliers ]

def fromdegrees(scale_degrees=None, octave=2, root='c', scale=None, ratios=None):
    if scale_degrees is None:
        scale_degrees = [1,3,5]

    if ratios is None:
        ratios = terry

    if scale is None:
        scale = major

    freqs = []
    root = ntf(root, octave, et)

    for index, degree in enumerate(scale_degrees):
        degree = int(degree)
        register = degree / (len(scale) + 1)
        chromatic_degree = scale[degree % len(scale) - 1]
        ratio = ratios[chromatic_degree]
        freqs += [ root * (ratio[0] / ratio[1]) * 2**register ]

    return freqs

def getQuality(name):
    quality = '-' if name.islower() else '^'
    quality = '*' if re.match(match_roman + '[*]', name) is not None else quality

    return quality

def getExtension(name):
    return re.sub(match_roman + '[/*+]?', '', name)

def getIntervals(name):
    quality = getQuality(name)
    extension = getExtension(name)

    chord = base_qualities[quality]

    if extension != '':
        chord = chord + extensions[extension]

    return chord

def getFreqFromChordName(name, root=440, octave=3, ratios=just):
    index = getChordRootIndex(name)
    freq = ratios[index]
    freq = root * (freq[0] / freq[1]) * 2**octave

    return freq

def stripChord(name):
    root = re.sub('[#b+/*^0-9]+', '', name)
    return root.lower()

def getChordRootIndex(name):
    root = chord_romans[stripChord(name)]

    if '#' in name:
        root += 1

    if 'b' in name:
        root -= 1

    return root % 12

def addIntervals(a, b):
    a = intervals[a]
    b = intervals[b]
    c = a + b
    for interval, index in intervals.iteritems():
        if c == index:
            c = interval

    return c

def getRatioFromInterval(interval, ratios):
    try:
        index = intervals[interval]
    except IndexError:
        log('Interval out of range, doh. Here have a P1')
        index = 0

    try:
        ratio = ratios[index]
        ratio = ratio[0] / ratio[1]
    except IndexError:
        base_index = index % 12
        ratio = ratios[base_index]
        ratio = ratio[0] / ratio[1]

        register = (index - base_index) / 12
        ratio *= 2**register

    return ratio

def nextChord(name):
    name = stripChord(name)
    return dsp.randchoose(progressions[name])

def chord(name, key=None, octave=3, ratios=None):
    if key is None:
        key = default_key

    if ratios is None:
        ratios = terry

    key = ntf(key, octave, ratios)
    root = ratios[getChordRootIndex(name)]
    root = key * (root[0] / root[1])

    name = re.sub('[#b]+', '', name)
    chord = getIntervals(name)
    chord = [ getRatioFromInterval(interval, ratios) for interval in chord ]
    chord = [ root * ratio for ratio in chord ]

    return chord

def chords(names, key=None, octave=3, ratios=just):
    if key is None:
        key = default_key

    return [ chord(name, key, octave, ratios) for name in names ]

def fit(freq, low=20, high=20000):
    """ fit the given freq within the given freq range, 
    by transposing up or down octaves """

    # need to fit at least an octave
    if high < low * 2:
        high = low * 2

    def shift(freq, low, high):
        if freq < low:
            return shift(freq * 2, low, high)

        if freq > high:
            return shift(freq * 0.5, low, high)

        return freq

    return shift(freq, low, high)
