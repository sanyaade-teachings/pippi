from pippi import dsp, oscs, tune

LOOP = False

def play(ctx):
    freqs = tune.degrees([dsp.randint(1,7)], key='c', octave=dsp.randint(3,4))
    stack = ['sine', 'hann', 'tri', 'cos', 'square']
    o = oscs.SineOsc(
            #stack,
            #windows=['sine', 'hann'],
            freq=dsp.choice(freqs),
            amp=dsp.rand(0.25, 0.5),
            pulsewidth=1,
        ).play(dsp.rand(1, 6)).env('rnd')
    o = o.pan(dsp.rand())

    yield o
