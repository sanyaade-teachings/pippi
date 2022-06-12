from pippi import dsp

LOOP = True

def play(ctx):
    out = ctx.adc(dsp.rand(0.5, 1), offset=2) 
    out = out.taper(0.05)
    yield out
