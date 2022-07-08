from pippi import dsp, fx, oscs

LOOP = True

def play(ctx):
    ctx.log('foo')
    #out = ctx.adc(dsp.rand(0.15, 1), offset=dsp.rand(0, 0.1)) 
    #out = fx.hpf(out, dsp.rand(100, 5000))
    out = oscs.SineOsc(freq=dsp.rand(100, 800)).play(1).env('pluckout') 
    out = out.taper(0.05).pan(dsp.rand())
    yield out * 0.2
