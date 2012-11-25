from pippi import dsp

def play(params={}):
    numcycles = dsp.randint(10, 1024)
    carrier = dsp.wavetable('vary', numcycles)
    mod = dsp.wavetable('cos2pi', numcycles)

    wtable = [ carrier[i] * mod[i] for i in range(numcycles) ]

    wtable = [ f * 16000 + 4000 for f in wtable ]

    wtable = [ dsp.cycle(f) for f in wtable ]

    return ''.join(wtable)

