from pippi import dsp, oscs

LOOP = True

def play(ctx):
    yield oscs.SineOsc(
            freq=dsp.rand(100, 500),
            amp=0.1
        ).play(out.dur).env('pluckout')
