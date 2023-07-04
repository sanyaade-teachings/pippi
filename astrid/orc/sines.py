from pippi import dsp, tune, fx, oscs, shapes

LOOP = True

def play(ctx):
    #minlength = ctx.m.cc(25, 1) * 10 + 10
    #maxlength = ctx.m.cc(26, 1) * 10 + minlength + 5
    minlength = 10
    maxlength = 15

    freqs = tune.chord('vi9', octave=dsp.randint(3,5))
    freq = dsp.choice(freqs)

    length = dsp.rand(minlength, maxlength)
    out = oscs.SineOsc(freq=freq).play(length)

    out = out.env('hann') * dsp.rand(0.8, 1) * 0.001

    out = out.pan(shapes.win('sine', length=dsp.rand(0.1, 0.5)))

    yield out
