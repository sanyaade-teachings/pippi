from pippi import dsp, oscs, tune, fx

import time

print("Lo! I have lo...aded")

c1 = [1,3,5]
c2 = [2,4,9]
c3 = [3,5,9]

def play(ctx):
    freqs = tune.degrees(dsp.choice([c1,c2,c3]), octave=dsp.randint(4,5))
    for _ in range(dsp.randint(1,4)):
        out = oscs.SineOsc(
            freq=dsp.choice(freqs)
            #freq=dsp.rand(200, 2000)
        ).play(dsp.rand(0.02, 0.1)).env('rnd').env('hannout').pan(dsp.rand()) * dsp.rand(0.25, 0.5)
        out = fx.fold(out, amp=dsp.rand(20,300))
        out = fx.norm(out, 0.15)
        yield out
