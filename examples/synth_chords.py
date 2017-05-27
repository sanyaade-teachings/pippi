import random
from pippi import dsp, oscs, tune

def make_wavetable():
    wtsize = random.randint(20, 100)
    wavetable = [0] + [ random.triangular(-1, 1) for _ in range(wtsize-2) ] + [0]
    return wavetable

osc = oscs.Osc(wavetable=make_wavetable())

out = dsp.buffer()

chords = 'ii vi V7 I69'

dubhead = 0
for chord in chords.split(' '):
    chordlength = int(random.triangular(44100 * 2, 44100 * 3))
    freqs = tune.chord(chord, key='e', octave=2)
    numnotes = random.randint(3, 6)

    notes = []

    for _ in range(numnotes):
        freq = random.choice(freqs)
        notelength = int(random.triangular(44100 * 3, 44100 * 4))
        osc.freq = freq
        osc.wavetable = make_wavetable()
        note = osc.play(notelength)
        note = note.env('sine') * random.triangular(0.1, 0.25)
        note = note.pan(random.random())

        out.dub(note, dubhead + random.randint(0, 1000))

    dubhead += chordlength

out.write('synth_chords.wav')
