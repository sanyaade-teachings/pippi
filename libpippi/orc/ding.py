from pippi import dsp, fx

LOOP = True

def play(ctx):
    #ctx.log('recorderrrr')
    adc = ctx.adc(1) 
    #adc.write('adc.wav')
    #ctx.log(len(adc))
    #ctx.log(adc.mag)
    out = fx.norm(adc, dsp.rand(0.5, 1))
    yield out
