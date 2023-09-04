from pippi import dsp, oscs, fx

#LOOP = True

def play(ctx):
    out = ctx.adc(10)
    out.write('adc.wav')
    yield None
