import random
import time
from pippi import dsp, oscs, tune, wavetables
import os

PATH = os.path.dirname(os.path.realpath(__file__))
print(__file__)
start_time = time.time()

tlength = 20
out = dsp.buffer(length=tlength)
pos = 0
count = 0
count2 = 0

def make_note(freq, amp, length):
    lfo = dsp.SINE
    lfo_freq = random.triangular(0.001, 15)

    # Frequency modulation wavetable
    mod = wavetables.randline(random.randint(10, 30))

    # Frequency of the freq mod wavetable
    mod_freq = random.triangular(0.001, 2)

    # Range / depth of the freq mod wavetable
    mod_range = random.triangular(0, 0.025)

    pulsewidth = random.random()

    # Fill up a stack of wavetables to give to the Osc.
    # The lfo wavetable above will control the morph position 
    # in the wavetable stack during the render.
    stack = []
    numtables = random.randint(3, 30)
    for _ in range(numtables):
        if random.random() > 0.25:
           stack += [ random.choice([dsp.SINE, dsp.TRI]) ]
        else:
           stack += [ wavetables.randline(random.randint(3, random.randint(5, 50))) ]

    # ADSR params
    a = random.triangular(0.1, 1)
    d = random.triangular(0.01, 3)
    s = random.triangular(0.03, 0.15)
    r = random.triangular(0.1, 10)

    # Render a note
    note = oscs.Osc(
                stack=stack, 
                window=dsp.SINE,
                mod=mod, 
                pulsewidth=pulsewidth, 
                mod_freq=mod_freq, 
                mod_range=mod_range, 
                lfo=lfo, 
                lfo_freq=lfo_freq
            ).play(length=length, freq=freq, amp=amp).adsr(a=a,d=d,s=s,r=r)

    # Sometimes mix in a granulated layer of the note
    if random.random() > 0.75:
        a = random.triangular(5, 10)
        d = random.triangular(0.1, 20)
        s = random.triangular(0.01, 0.1)
        r = random.triangular(5, 10)

        arp = note.cloud(length * random.triangular(1.5, 3), 
                    win=dsp.HANN, 
                    minlength=(note.dur/50)*1000, 
                    maxlength=(note.dur/3)*1000, 
                    grainlength_lfo=wavetables.randline(random.randint(10, 100)),
                    mindensity=0.1, 
                    maxdensity=random.triangular(0.25, 0.75), 
                    spread=1, 
                    speed=random.choice([1, 1.5, 2]),
                    jitter=random.triangular(0, 0.05),
                    density_lfo=wavetables.randline(random.randint(5, 50)),
                ).adsr(a,d,s,r)
        note.dub(arp, 0)

    return note


def get_freqs(count):
    # mahjong by wayne shorter
    chords = (['i11', 'VII69'] * 6) + (['VI9', 'VII69'] * 4) + (['VI7', 'vii7', 'III7', 'VI9', 'vi7', 'II7']) + (['i11', 'VII69'] * 6)
    chords = ['i','i','i','I','I','I69','i','i','i']
    return tune.chord(chords[count % len(chords)], octave=2)

freqs = get_freqs(0)
beat = 1

blens = [] 
blens += [beat * 0.25] * 5 
blens += [beat * 0.5] * 25 
blens += [beat] * 25 

random.seed(1992)
drift = 0 
while pos < tlength:
    length = random.choice(blens)
    freqs = get_freqs(count // 3)
    params = []
    reps = random.choice([1,2,3,4,8])
    dist = random.choice(blens) / reps

    for _ in range(reps):
        freq = freqs[(count2 // random.randint(1,3)) % len(freqs)] * 2**random.randint(0, 3)
        amp = random.triangular(0.05, 0.15)
        note = make_note(freq, amp, length)
        out.dub(note, pos)
        count2 += 1
        pos += dist + drift
        drift += random.triangular(-0.002, 0.005)

    count += 1

out.write('%s/osc2d_out.wav' % PATH)

elapsed_time = time.time() - start_time
print('Render time: %s seconds' % round(elapsed_time, 2))
print('Output length: %s seconds' % out.dur)
