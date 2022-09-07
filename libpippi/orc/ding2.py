from pippi import dsp, oscs, tune, fx

LOOP = True

def play(ctx):
    cc1 = ctx.p.cc1 or 0

    freqs = tune.degrees([1,3,5,6,7,9], octave=dsp.randint(3,4), key='f')
    l = dsp.rand(0.25, 2)
    #lfo = dsp.win('rnd', 0.99, 1.02) * dsp.win('rnd')
    out = oscs.SineOsc(
        freq=dsp.choice(freqs) * 1
    ).play(l).env('hann') * dsp.rand(0.8, 1)
    #out = fx.crush(out, bitdepth=8, samplerate=dsp.rand(1000,44100))

    #boop = dsp.win('rnd').repeated(7)

    yield out * dsp.rand(0.001, 0.01)
