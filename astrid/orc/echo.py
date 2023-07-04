from pippi import dsp, oscs, fx

LOOP = True

def play(ctx):
    if dsp.rand() > 0.999:
        length = dsp.rand(0.1, 1)
    else:
        length = dsp.rand(0.01, 0.15)
    offset = dsp.rand(0, 1)

    out = ctx.adc(length, offset=offset).env('hann')
    yield out
