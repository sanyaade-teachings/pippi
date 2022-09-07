from pippi import dsp, fx, oscs, shapes

LOOP = True

def play(ctx):
    ctx.log('recorderrrr')
    adc = ctx.adc(1000) 
    adc.write('adc.wav')
    ctx.log(len(adc))
    ctx.log(adc.mag)
    out = fx.norm(adc, dsp.rand(0.5, 0.85))
    yield out
