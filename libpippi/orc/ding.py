from pippi import dsp, oscs, tune

import time

dsp.seed(time.time())

print("Lo! I have lo...aded")

def play(ctx):
    freqs = tune.degrees([1,3,5,9], octave=5)
    freq = dsp.choice(freqs)
    print(freq)
    out = oscs.SineOsc(
        freq=freq
    ).play(dsp.rand(0.1, 0.3)).env('hannout').pan(dsp.rand()) * 0.8
    yield out
