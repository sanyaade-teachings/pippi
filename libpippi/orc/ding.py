from pippi import dsp, oscs, fx

LOOP = True

def play(ctx):
    ctx.log(ctx.p.g)
    o = oscs.Tukey(
            freq=dsp.rand(100, dsp.rand(50, 1000)),
            amp=dsp.rand(0.15, 0.8),
            shape=dsp.rand(),
        ).play(dsp.rand(0.15, 3)).env('pluckout')

    #o = fx.crush(o, bitdepth=dsp.rand(3, 8), samplerate=dsp.rand(1000, 20000))
    #o = fx.fold(o, dsp.rand(1,3))

    yield o
