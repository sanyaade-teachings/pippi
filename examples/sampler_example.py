import random
from pippi import dsp, sampler, tune, rhythm

samp = sampler.Sampler('sounds/harpc2.wav', 'c2')

out = dsp.buffer()
chords = ['iii', 'vi', 'ii', 'V']

def arp(i):
    cluster = dsp.buffer()
    length = random.randint(44100, 44100 + 22050)
    numnotes = random.randint(3, 12)
    onsets = rhythm.curve(numnotes, 'random', length)
    chord = chords[i % len(chords)]
    freqs = tune.chord(chord, octave=random.randint(1, 3))
    for i, onset in enumerate(onsets): 
        freq = freqs[i % len(freqs)]
        note = samp.play(freq)
        note = note.pan(random.random())
        note *= random.triangular(0, 0.125)
        cluster.dub(note, onset)        

    return cluster

pos = 0
for i in range(20):
    for _ in range(random.randint(1,3)):
        cluster = arp(i)
        out.dub(cluster, pos)

    pos += 44100

out.write('sampler_example.wav')
