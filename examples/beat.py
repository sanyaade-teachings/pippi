from pippi import dsp, rhythm
import random

out = dsp.silence()

snare = dsp.read('sounds/snare.wav')

numbeats = 128
pattern = rhythm.curve(numbeats, 'sine', div=44100//32)

for onset in pattern:
    out.dub(snare * random.random())

out.write('beat_example.wav')
