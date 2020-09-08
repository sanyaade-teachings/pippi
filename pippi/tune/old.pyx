import re
import math
import random

a0 = 27.5 
a4 = a0 * 2**4
DEFAULT_KEY = 'c'

INTERVALS = {
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

EQUAL = tuple([ 2**(i/12) for i in range(12) ])

JUST = ( 
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

TERRY = (
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

YOUNG = (
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

LOUIS = (
    (1, 1), 
    (math.sqrt(5) * 0.5, 1),
    (math.sqrt(6) * 0.5, 1), 
    (math.sqrt(7) * 0.5, 1), 
    (math.sqrt(2), 1), 
    (math.sqrt(9) * 0.5, 1), 
    (math.sqrt(10) * 0.5, 1), 
    (math.sqrt(11) * 0.5, 1), 
    (math.sqrt(3), 1), 
    (math.sqrt(13) * 0.5, 1), 
    (math.sqrt(14) * 0.5, 1), 
    (math.sqrt(15) * 0.5, 1), 
)


## scale mappings
# standard
MAJOR = (0, 2, 4, 5, 7, 9, 11)
MINOR = (0, 2, 3, 5, 7, 8, 10)
CHROMATIC = tuple(range(12))

# notated as semitone deviations from 
# a MAJOR scale for readability
SCALES = {
    'MAJOR': MAJOR,
    'MINOR': MINOR,
    'super_locrian': (0, 2-1, 4-1, 5-1, 7-1, 9-1, 11-1),
    'neapolitan_minor': (0, 2-1, 4-1, 5, 7, 9-1, 11),
    'neapolitan_major': (0, 2-1, 4-1, 5, 7, 9, 11),
    'oriental': (0, 2-1, 4, 5, 7-1, 9, 11-1),
    'double_harmonic': (0, 2-1, 4, 5, 7, 9-1, 11),
    'enigmatic': (0, 2-1, 4, 5+1, 7+1, 9+1, 11),
}

# Maps to CHROMATIC ratio lists above
NOTES = { 
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

MIDI_NOTES = {
    'a': 21,
    'a#': 22,
    'bb': 22,
    'b': 23, 
    'c': 24, 
    'c#': 25,
    'db': 25,
    'd': 26,
    'd#': 27,
    'eb': 27,
    'e': 28, 
    'f': 29, 
    'f#': 30, 
    'gb': 30, 
    'g': 31, 
    'g#': 32,
    'ab': 32,
}

DEFAULT_RATIOS = TERRY
DEFAULT_SCALE = MAJOR


def parse_pitch_class(note, octave=None):
    note = note.lower()
    if re.match('[a-zA-Z]#?b?\d+', note) is not None:
        parsed = re.match('([a-zA-Z]#?b?)(\d+)', note)
        note = parsed.group(1)
        octave = int(parsed.group(2))

    if octave is None:
        octave = 4

    return note, octave

def ntm(note, octave=None):
    note, octave = parse_pitch_class(note, octave)
    note_index = nti(note)
    if note_index >= 3:
        octave -= 1
    return note_index + 21 + (octave * 12)

def edo(degree, divs=12):
    return 2**(degree/float(divs))

def edo_ratios(divs=12):
    return [ (edo(i, divs), 1.0) for i in range(1, divs+1) ]

def edo_scale(divs=12):
    return [ edo(i, divs) for i in range(1, divs+1) ]

def nti(note):
    """ Note to index
            returns the index of enharmonic note names
            or False if not found
    """
    return NOTES.get(note, False)

def ntf(note, octave=None, ratios=None):
    """ Note to freq 
    """
    if ratios is None:
        ratios = DEFAULT_RATIOS

    note, octave = parse_pitch_class(note, octave)

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

    degree = NOTES[note] * 2

    degree = degree + ((octave + 2) * 24) 

    return degree

def getmultiplier(ratios, scale, degree):
    base = scale[(degree - 1) % len(scale)]
    octave = degree // (len(scale) + 1)
    mult = ratios[base]
    if isinstance(mult, tuple):
        mult = mult[0] / mult[1]
    mult = mult * 2**octave
    return mult

def int_to_byte_list(val):
    return list(map(int, list(bin(val))[2:]))

def str_to_byte_list(val):
    return list(map(int, val))

def to_scale_mask(mapping):
    mask = []
    if isinstance(mapping, int):
        mask = int_to_byte_list(mapping)
    elif isinstance(mapping, bytes):
        for i in list(mapping):
            mask += int_to_byte_list(i)
    elif isinstance(mapping, str):
        mask = str_to_byte_list(mapping)
    elif isinstance(mapping, list) or isinstance(mapping, tuple):
        mask = list(map(int, mapping))
    else:
        raise NotImplemented

    return mask

def scale_mask_to_indexes(mask):
    mask = to_scale_mask(mask)
    scale_indexes = []
    for i, m in enumerate(mask):
        if m == 1:
            scale_indexes += [ i ]
    return scale_indexes

def tofreqs(degrees=None, root=261.63, ratios=None, scale=None, scale_mask=None):
    if ratios is None:
        ratios = DEFAULT_RATIOS

    if scale is None:
        scale = DEFAULT_SCALE
    
    if degrees is None:
        degrees = range(len(scale))

    if scale_mask is not None:
        scale = scale_mask_to_indexes(scale_mask)

    freqs = []
    for degree in degrees:
        freq = root * getmultiplier(ratios, scale, degree)
        freqs += [ freq ]

    return freqs

def fromdegrees(scale_degrees=None, octave=2, root='c', scale=None, ratios=None):
    # TODO maybe depricate in favor of to_freqs()
    if scale_degrees is None:
        scale_degrees = [1,3,5]

    if ratios is None:
        ratios = DEFAULT_RATIOS

    if scale is None:
        scale = MAJOR

    freqs = []
    root = ntf(root, octave)

    for index, degree in enumerate(scale_degrees):
        # strings are okay
        degree = int(degree)

        # register offset 0+
        register = degree // (len(scale) + 1)

        chromatic_degree = scale[(degree - 1) % len(scale)]
        ratio = ratios[chromatic_degree]
        freqs += [ root * (ratio[0] / ratio[1]) * 2**register ]

    return freqs

def apply_interval(freq, iv=None, ratios=None):
    if iv is None:
        iv = 'P1'
    if ratios is None:
        ratios = DEFAULT_RATIOS
    return freq * get_ratio_from_interval(iv, ratios)

def add_intervals(a, b):
    a = INTERVALS[a]
    b = INTERVALS[b]
    c = a + b
    for iv, index in INTERVALS.items():
        if c == index:
            return iv
    return None

def get_ratio_from_interval(iv, ratios):
    index = INTERVALS[iv]

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

def fit_scale(freq, scale):
    # Thanks to @kennytm from stack overflow for this <3 <3
    return min(scale, key=lambda x:abs(x-freq))

def fit(freq, low=20, high=20000, get_change=False):
    """ fit the given freq within the given freq range, 
    by transposing up or down octaves """

    # need to fit at least an octave
    if high < low * 2:
        high = low * 2

    def shift(freq, low, high, octave_shift=0):
        if freq < low:
            return shift(freq * 2, low, high, octave_shift + 1)

        if freq > high:
            return shift(freq * 0.5, low, high, octave_shift - 1)

        return freq, octave_shift

    freq, octave = shift(freq, low, high)

    if octave == 0:
        mult = 1
    elif octave > 0:
        mult = 2**octave
    elif octave < 0:
        mult = 1.0 / (2**abs(octave))

    if get_change == True:
        return freq, mult
    else:
        return freq

class Parser:
    def __init__(self, score, durations=None, octave=3, ratios=None):
        self.score = score
        self.octave = octave

        if durations is None:
            durations = (0.5,)
        self.durations = durations

        if ratios is None:
            ratios = DEFAULT_RATIOS
        self.ratios = ratios

    def parse(self):
        pos = 0
        events = []

        for note in self.score.split(' '):
            cluster = note.split(',')
            events += [(pos, cluster)]
            

        #self.parsed = parsed
