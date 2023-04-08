from pippi import dsp

LOOP = True

def play(ctx):
    # almost!
    yield ctx.adc(0.1) * 0.1
