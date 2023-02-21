from pippi import dsp, oscs, fx

LOOP = True

def play(ctx):
    length = dsp.rand(0.5, 2)
    out = ctx.adc(length)

    freq = dsp.rand(200, 2000)
    ctx.log('Ringmod freq %s' % freq)

    out = fx.norm(out, 1)
    out = out.env(oscs.SineOsc(freq=freq).play(length))
    out = fx.norm(out, dsp.rand(0.2, 0.5))
    out = out.env('pluckout')

    yield out
