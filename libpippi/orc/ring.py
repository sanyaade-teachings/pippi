from pippi import dsp, oscs, fx

LOOP = True

def play(ctx):
    length = dsp.rand(0.1, 3)
    out = ctx.adc(length)

    ctx.log('Voice ID %s' % ctx.id)

    freq = dsp.rand(1000, 10000)
    ctx.log('Ringmod freq %s' % freq)
    out = out.env(oscs.SineOsc(freq=freq).play(length).pan(dsp.rand()))

    out = fx.fold(out, dsp.rand(1, 2))
    out = out.env('rnd')

    if dsp.rand() > 0.5:
        out = out.env(dsp.win('rnd').repeated(dsp.randint(5, 100)))

    if dsp.rand() > 0.5:
        out = out.env('rnd')

    out = fx.lpf(out, dsp.rand(100, 10000))
    out = out.vspeed(dsp.win('rnd', dsp.rand(0.85, 1.15), dsp.rand(2, 5)))

    out = fx.norm(out, dsp.rand(0.2, 0.75))

    yield out
