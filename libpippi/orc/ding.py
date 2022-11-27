from pippi import dsp, oscs

#LOOP = True

def play(ctx):
    o = oscs.SineOsc(
            freq=dsp.rand(500, 800),
            amp=dsp.rand(0.15, 0.45),
        ).play(0.04).env('pluckout')

    yield o
