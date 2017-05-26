from pippi import dsp, rhythm
import random

out = dsp.buffer()

snare = dsp.read('sounds/snare.wav')

pattern = rhythm.curve(numbeats=32, wintype='sine', div=44100//32, mult=100)

for onset in pattern:
    out.dub(snare * random.random(), onset)

out.write('beat_example.wav')
