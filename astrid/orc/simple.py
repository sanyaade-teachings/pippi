from pippi import dsp, oscs, renderer

def trigger(ctx):
    #return None
    events = []
    events += [ ctx.t.play(0, 'simple') ]
    events += [ ctx.t.update(0, 'littlefield', 'gamp=%s' % dsp.rand(0,1)) ]
    events += [ ctx.t.trigger(2, 'simple') ]
    return events

def update(ctx, k, v):
    ctx.log('k: %s v: %s' % (k, v))
    ctx.t.update(0, 'littlefield', 'gamp=%s' % dsp.rand(0,1)).schedule()

def play(ctx):
    ctx.log('Rendering simple tone!')
    yield oscs.SineOsc(freq=530, amp=0.5).play(1).env('pluckout')

if __name__ == '__main__':
    renderer.run_forever(__file__)
