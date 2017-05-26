from pippi import dsp, rhythm
import random

out = dsp.buffer()

snare = dsp.read('sounds/snare.wav')

numpasses = random.randint(4, 8)

for _ in range(numpasses):
    numbeats = random.randint(16, 64)
    reverse = random.choice([True, False])
    wintype = random.choice(['sine', 'tri', 'kaiser', 'hann', 'blackman'])
    div = 44100//random.randint(16, 64)
    mult = random.randint(50, 300)
    pattern = rhythm.curve(numbeats=numbeats, wintype=wintype, div=div, mult=mult, reverse=reverse)
    minspeed = random.triangular(0.15, 2)
    pan = random.random()

    for onset in pattern:
        hit = snare * random.triangular(0, 0.5)
        hit = hit.speed(random.triangular(minspeed, minspeed + 0.08))
        hit = hit.pan(pan)
        out.dub(hit, onset)

out.write('multi_snare_bounce_example.wav')
