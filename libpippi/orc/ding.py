from pippi import dsp, oscs, tune

import time

dsp.seed(time.time())

print("Lo! I have lo...aded")

def play(ctx):
    freqs = tune.degrees([1,3,5,9] + [ dsp.randint(1, 20) for _ in range(10) ], octave=dsp.randint(3,8))
    freq = dsp.choice(freqs)
    print(freq)
    out = oscs.SineOsc(
        freq=freq
    ).play(dsp.rand(0.01, 0.1)).env('rnd').pan(dsp.rand()).taper(dsp.MS*2) * 0.8
    yield out
