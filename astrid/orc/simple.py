from pippi import dsp, oscs, renderer, fx

def trigger(ctx):
    return [ctx.t.play(0, 'simple'), ctx.t.trigger(dsp.rand(0.15, 0.5), 'simple')]

def play(ctx):
    ctx.log('Rendering simple tone!')
    out = oscs.SineOsc(freq=dsp.rand(200, 1000), amp=dsp.rand(0.1, 0.5)).play(10).env('pluckout')
    #adc = fx.lpf(ctx.adc(out.dur).env('rnd'), dsp.rand(100, 400)).speed(dsp.rand(0.1, 0.25))
    #if dsp.rand() > 0.5:
    #    adc = fx.fold(adc, dsp.rand(1, 2))

    #out = out & adc

    yield fx.norm(out, 0.5)

if __name__ == '__main__':
    renderer.run_forever(__file__)
