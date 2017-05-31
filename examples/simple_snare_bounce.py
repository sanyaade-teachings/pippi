from pippi import dsp, rhythm
import random

out = dsp.buffer()

snare = dsp.read('sounds/snare.wav')

pattern = rhythm.curve(numbeats=64, wintype='sine', length=44100 * 4)

for onset in pattern:
    out.dub(snare * random.random(), onset)

out.write('beat_example.wav')
