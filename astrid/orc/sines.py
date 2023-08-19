from pippi import dsp, tune, fx, oscs, shapes

#LOOP = True

def trigger(ctx):
    loop = 6

    events = []
    pos = 0
    for _ in range(dsp.randint(3,5)):
        events += [ ctx.t.play(pos, 'ding') ]
        #pos += 0.3333
        pos += dsp.rand(0.01, 0.3)
    events += [ ctx.t.play(0, 'ding') ]
    #events += [ ctx.t.play(0, 'osc') ]
    #events += [ ctx.t.trigger(loop, 'sines') ]
    return events

def play(ctx):
    #minlength = ctx.m.cc(25, 1) * 10 + 10
    #maxlength = ctx.m.cc(26, 1) * 10 + minlength + 5
    minlength = 1
    maxlength = 3

    length = dsp.rand(minlength, maxlength)
    freqs = tune.chord('I7', key='f', octave=dsp.randint(4,7))
    freq = freqs[0]
    freq = dsp.choice(freqs)
    diff = dsp.rand(0.3, 1)
    freq = shapes.win('sine', freq - diff, freq + diff, length=length * dsp.rand(1, 10))

    out = oscs.SineOsc(freq=freq).play(length)
    out = fx.fold(out, shapes.win('sine', 20, 30, length=1))
    out = fx.crush(out, bitdepth=dsp.rand(4, 16), samplerate=dsp.rand(4000, 16000))

    #out = out.env(dsp.choice(['hannout', 'pluckout']))
    #out = out.env('pluckout')
    out = out.env('sineout')
    out = out.pan(shapes.win('sine', length=dsp.rand(0.1, 0.5)))

    out = out.env(dsp.win('hann', 0.3, 1).repeated(dsp.randint(3, 30)))

    out = fx.lpf(out, dsp.rand(200, 800))
    #out = fx.compressor(out * 3, -15, 15)
    out = fx.norm(out, dsp.rand(0.35, 0.75))
    #out = out.pad(end=dsp.rand(0, 4))

    yield out
