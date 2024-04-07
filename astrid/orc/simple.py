from pippi import dsp, oscs, renderer, fx

def trigger(ctx):
    return [ctx.t.play(0, 'simple'), ctx.t.trigger(dsp.rand(0.5, 1), 'simple')]

def play(ctx):
    ctx.log('Rendering simple tone!')
    out = oscs.SineOsc(freq=dsp.rand(200, 1000), amp=dsp.rand(0.1, 0.5)).play(10).env('pluckout')
    out = out & ctx.adc(out.dur).env('pluckout')
    #out = ctx.adc(1)

    #if dsp.rand() > 0.5:
    #    out = fx.fold(out, dsp.rand(1, 2))
    yield fx.norm(out, 0.2)

if __name__ == '__main__':
    renderer.run_forever(__file__)
