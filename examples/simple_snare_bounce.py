from pippi import dsp, rhythm
import random
import os

PATH = os.path.dirname(os.path.realpath(__file__))
print(__file__)

out = dsp.buffer()

snare = dsp.read('%s/sounds/snare.wav' % PATH)

pattern = rhythm.curve(numbeats=64, wintype=dsp.SINE, length=44100 * 4)

for onset in pattern:
    out.dub(snare * random.random(), onset/out.samplerate)

out.write('%s/simple_snare_bounce.wav' % PATH)
