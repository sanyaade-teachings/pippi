from pippi import dsp

shortname       = 'sl'
name            = 'slurp'
device          = 'default'
loop            = True

def play(params={}):
    numcycles = dsp.randint(10, 524)

    curve_a = dsp.breakpoint([1.0] + [dsp.rand(0.1, 1.0) for r in range(dsp.randint(2, 10))] + [0], numcycles)
    curve_types = ['vary', 'phasor', 'sine', 'cos', 'impulse']
    curve_b = dsp.wavetable(dsp.randchoose(curve_types), numcycles)

    pan = dsp.wavetable('cos', numcycles)

    wtable = [ curve_a[i] * curve_b[i] for i in range(numcycles) ]
    wtable = [ (f * 19000) + 40 for f in wtable ]

    #wtypes = ['impulse', 'tri', 'cos', 'sine2pi']
    out = [ dsp.pan(dsp.cycle(wtable[i], 'sine2pi'), pan[i]) for i in range(numcycles) ]

    return ''.join(out)

