from pippi import dsp

a0 = 27.5 

just = [
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
]

vary = [ (dsp.rand(-0.05, 0.05) + i[0], dsp.rand(-0.05, 0.05) + i[1]) for i in just ]

terry = [
    (1.0, 1.0),     # P1
    (16.0, 15.0),   # m2
    (10.0, 9.0),    # M2
    (6.0, 5.0),     # m3
    (5.0, 4.0),     # M3
    (4.0, 3.0),     # P4
    (64.0, 45.0),   # TT
    (3.0, 2.0),     # P5
    (8.0, 5.0),     # m6
    (27.0, 16.0),   # M6
    (16.0, 9.0),    # m7
    (15.0, 8.0),    # M7
]

young = [
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
]

# Handy subsets of the chromatic scale
major = [0, 2, 4, 5, 7, 9, 11]
minor = [0, 2, 3, 5, 7, 8, 10]

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

def ntf(note, octave=4, ratios=terry):
    """ Note to freq 
    """
    return ratios[nti(note)][0] / ratios[nti(note)][1] * (a0 * (2.0**octave))

def stf(index):
    degree = index % 24
    octave = index / 24

    return (2 ** (degree / 24.0)) * (a0 / 4.0) * (2.0 ** octave)

def fts(freq):
    # Try to find closest eq temp freq to input
    # Generate entire range of possible eq temp freqs
    all_freq = [ stf(index) for index in range(2**8) ]

    count = 0
    cfreq = 0
    while freq > cfreq and count < 2**8:
        cfreq = all_freq[count]
        count = count + 1

    return count - 1

def nts(note, octave):
    octave = octave if octave >= -2 else -2
    octave = octave if octave <= 8 else 8 

    degree = notes[note] * 2

    return degree + ((octave + 2) * 24) 

def fromdegrees(scale_degrees=[1,3,5], octave=2, root='c', scale=major, ratios=just):
    freqs = []
    root = ntf(root, octave, ratios)

    for index, degree in enumerate(scale_degrees):
        degree = int(degree)
        register = degree / len(scale)
        chromatic_degree = scale[degree % len(scale) - 1]
        ratio = ratios[chromatic_degree]
        freqs += [ root * (ratio[0] / ratio[1]) * 2**register ]

    return freqs

def step(degree=0, root='c', octave=4, scale=[1,3,5,8], quality=major, ratios=terry):
    # TODO. Many qualities of jank. Fix.

    diatonic = scale[degree % len(scale) - 1]
    chromatic = quality[diatonic % len(quality) - 1]

    pitch = ratios[chromatic][0] / ratios[chromatic][1]
    pitch *= octave + int(diatonic / len(quality))
    pitch *= ntf(root, octave, ratios)

    return pitch

def scale(pitches=[1,3,5], quality=major):
    return [quality[p - 1] for p in pitches]
