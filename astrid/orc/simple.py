from pippi import dsp, oscs, renderer, fx, shapes, tune

def trigger(ctx):
    return None
    #onset = dsp.rand(0.01, dsp.rand(0.4, dsp.rand(0.8, 1)))
    #onset = dsp.choice([0.1, 0.2, 0.3]) * 2.25
    onset = 3
    events = []

    pos = 0
    while pos < onset:
        events += [ ctx.t.play(pos, 'simple') ]
        events += [ ctx.t.serial(pos+0.05, 'simple', 's %d' % dsp.randint(0, 4)) ]
        pos += dsp.rand(0.01, 0.14)
    events += [ ctx.t.trigger(onset, 'simple') ]
    return events

def play(ctx):
    ctx.log('Rendering simple tone!')
    length = dsp.rand(0.01, dsp.rand(0.5, dsp.rand(1.4, 2)))
    #length = 2
    pw = shapes.win('sine', dsp.rand(0, 0.8), 1)

    """
    adcpass = ctx.adc(length)
    sbase = dsp.rand(0.1, 0.8)
    adc = adcpass.vspeed(shapes.win('sine', sbase, sbase+0.2)).rcut(length)
    #adc = fx.fold(adc, dsp.rand(2, 5))
    fbase = dsp.rand(100, 10000)
    adc = fx.bpf(adc, shapes.win('sine', fbase, fbase + 100))
    adc = fx.norm(adc, 1).pan(shapes.win('sine'))

    wts = dsp.ws(adc.rcut(0.2))
    wts.normalize()
    """
    wts = ['rnd'] * 10

    freqs = tune.degrees([ dsp.choice([1,3,5,6,8]) for _ in range(dsp.randint(3,6)) ], octave=4, key='d')
    #freqs = [22]

    out = dsp.buffer(length=length)
    for freq in freqs:
        o = oscs.Pulsar2d(wts, pulsewidth=pw, freq=freq * 2**dsp.randint(0,3), amp=dsp.rand(0.2, 0.5)).play(length)
        out.dub(o)
    #out = out & out.env(adc)
    #out = fx.bpf(out, shapes.win('sine', 100, 10000)) & (adcpass.env('hann') * 0.1)

    #yield fx.norm(out, dsp.rand(0.75, 0.85)).env(shapes.win('rnd', dsp.rand(0, 0.8), 1)).env('pluckout')
    yield fx.norm(out, dsp.rand(0.75, 0.85)).env('pluckout')

if __name__ == '__main__':
    renderer.run_forever(__file__)
