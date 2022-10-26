from pippi import dsp, fx, oscs

LOOP = True

def play(ctx):
    out = ctx.adc(dsp.rand(1, 3)) 
    #ctx.log(dsp.rand())
    #out.write('adc.wav')
    #out = fx.bpf(out, dsp.rand(100, 10000))
    #out = fx.hpf(out, 200)
    out = fx.norm(out, dsp.rand(0.5, 0.6))
    #out.dub(oscs.SineOsc(freq=dsp.choice([100,200,300,400,500])).play(out.dur) * 0.05)
    out = out.env('sine')
    yield out
