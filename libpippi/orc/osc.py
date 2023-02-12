from pippi import dsp, oscs, tune

#LOOP = True


def before(ctx):
    ctx.log('before firing')

def play(ctx):
    length = ctx.cache.get('length', 1)
    note = float(ctx.p.note or 60)
    ctx.log('note %s' % note)
    freq = tune.mtf(note)

    osc = oscs.SineOsc(freq=freq, amp=0.05)
    ctx.log('length %s' % length)

    yield osc.play(length).taper(dsp.rand(dsp.MS, 0.1))

def done(ctx):
    ctx.log('done firing')
    ctx.cache['length'] = dsp.rand(0.1,1)
