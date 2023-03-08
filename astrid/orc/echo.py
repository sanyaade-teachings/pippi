from pippi import dsp, oscs, fx

LOOP = True

def play(ctx):
    length = dsp.rand(0.5, 3)
    out = ctx.adc(length).env('rnd')
    yield out
