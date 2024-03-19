from pippi import dsp, oscs, renderer

def play(ctx):
    ctx.log('Rendering simple tone!')
    yield oscs.SineOsc(freq=dsp.rand(200, 300), amp=1).play(10).env('pluckout')

if __name__ == '__main__':
    renderer.run_forever(__file__)
