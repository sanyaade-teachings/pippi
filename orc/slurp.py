from pippi import dsp

def play(params={}):
    numcycles = dsp.randint(10, 524)

    curve_a = dsp.breakpoint([1.0] + [dsp.rand(0.1, 1.0) for r in range(dsp.randint(2, 10))] + [0], numcycles)
    curve_b = dsp.wavetable('cos', numcycles)

    pan = dsp.wavetable('vary', numcycles)

    wtable = [ curve_a[i] * curve_b[i] for i in range(numcycles) ]
    wtable = [ (f * 11000) + 70 for f in wtable ]

    out = [ dsp.pan(dsp.cycle(wtable[i]), pan[i]) for i in range(numcycles) ]

    return ''.join(out)

