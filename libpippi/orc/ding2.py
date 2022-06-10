from pippi import dsp, oscs, tune, fx

import time

c1 = [1,3,5]
c2 = [2,4,9]
c3 = [3,5,9]

def play(ctx):
    cc1 = ctx.p.cc1 or 0
    cc2 = ctx.p.cc2 or 0
    cc3 = ctx.p.cc3 or 0
    cc4 = ctx.p.cc4 or 0
    cc5 = ctx.p.cc5 or 0

    freqs = tune.degrees([1,3,5,6], octave=dsp.randint(2,6), key='f')
    for _ in range(dsp.randint(3,4)):
        l = cc1 * 8 + 0.01
        lfo = dsp.win('rnd', 0.99, 1.02) * dsp.win('rnd')
        out = oscs.SineOsc(
            freq=dsp.choice(freqs) * lfo,
        ).play(l).env('hann') * dsp.rand(0.8, 1)
        out = fx.crush(out, bitdepth=8, samplerate=dsp.rand(1000,44100))

        boop = dsp.win('rnd').repeated(7)

        yield out * boop
