from pippi import dsp, oscs, renderer, fx, ugens

def stream(ctx):
    graph = ugens.Graph()
    graph.add_node('s0b', 'sine', freq=0.1)
    graph.add_node('s0a', 'sine', freq=0.1)
    graph.add_node('s0', 'sine', freq=100)
    graph.add_node('s1', 'sine')
    graph.add_node('s2', 'sine')

    graph.connect('s0b.output', 's0a.freq', 0.1, 200)
    graph.connect('s0a.output', 's0.freq', 100, 300)
    graph.connect('s0.output', 's1.freq', 0.1, 1)
    graph.connect('s0.output', 'main.output', mult=0.1)
    graph.connect('s1.output', 's2.freq', 60, 100)
    graph.connect('s2.output', 'main.output', mult=0.2)
    graph.connect('s2.freq', 's0b.freq', 0.15, 0.5, 60, 100)

    return graph

def trigger(ctx):
    return [ctx.t.play(0, 'simple'), ctx.t.trigger(dsp.rand(0.5, 1), 'simple')]

def play(ctx):
    ctx.log('Rendering simple tone!')
    out = oscs.SineOsc(freq=dsp.rand(200, 1000), amp=dsp.rand(0.1, 0.5)).play(10).env('pluckout')
    out = out & ctx.adc(out.dur)

    if dsp.rand() > 0.5:
        out = fx.fold(out, dsp.rand(1, 2))
    yield out

if __name__ == '__main__':
    renderer.run_forever(__file__)
