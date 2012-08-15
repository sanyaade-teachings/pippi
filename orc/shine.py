import dsp
import tune

def play(args):
    length = dsp.stf(dsp.rand(5, 10))
    volume = 0.2 
    octave = 2 
    note = 'd'
    quality = tune.major
    reps = dsp.randint(3, 11)
    glitch = False
    superglitch = False
    glitchpad = 0
    glitchenv = False
    pulse = False
    env = False
    ratios = tune.terry

    instrument = 'rhodes'
    tone = dsp.read('sounds/220rhodes.wav').data

    #scale = [1,3,5,4]
    scale = [1,5,8]

    for arg in args:
        a = arg.split(':')

        if a[0] == 't':
            length = dsp.stf(float(a[1]))

        if a[0] == 'v':
            volume = float(a[1]) / 100.0

        if a[0] == 'o':
            octave = int(a[1])

        if a[0] == 'n':
            note = a[1]

        if a[0] == 'i':
            if a[1] == 'r':
                instrument = 'rhodes'
                tone = dsp.read('sounds/220rhodes.wav').data
            elif a[1] == 'c':
                instrument = 'clarinet'
                tone = dsp.read('sounds/clarinet.wav').data
            elif a[1] == 'g':
                instrument = 'guitar'
                tone = dsp.mix([dsp.read('sounds/guitar.wav').data, 
                                dsp.read('sounds/banjo.wav').data])


        if a[0] == 'q':
            if a[1] == 'M':
                quality = tune.major
            elif a[1] == 'm':
                quality = tune.minor
            else:
                quality = tune.major

        if a[0] == 'r':
            reps = int(a[1])

        if a[0] == 'p':
            pulse = True

        if a[0] == 'g':
            glitch = True

        if a[0] == 'gg':
            glitch = True
            superglitch = int(a[1]) if len(a) > 1 else 100

        if a[0] == 'e':
            env = a[1] if len(a) > 1 else 'sine'

        if a[0] == 'gp':
            glitchpad = int(a[1])

        if a[0] == 'ge':
            glitchenv = True

        if a[0] == 'tr':
            ratios = getattr(tune, a[1], tune.terry)

        if a[0] == 's':
            scale = [int(s) for s in a[1].split('.')]

    out = ''
    for i in range(reps):
        freq = tune.step(i, note, octave, scale, quality, ratios)

        if instrument == 'clarinet':
            diff = freq / 293.7
        else:
            diff = freq / 440.0

        n = dsp.transpose(tone, diff)

        if env is not False:
            n = dsp.env(n, env)

        n = dsp.fill(n, length)

        o = [dsp.tone(length, freq * i * 0.5) for i in range(4)]
        o = [dsp.env(oo) for oo in o]
        o = [dsp.pan(oo, dsp.rand()) for oo in o]
        o = dsp.mix([dsp.amp(oo, dsp.rand(0.4, 0.9)) for oo in o])


        if env is not False:
            o = dsp.env(o, env)


        out += dsp.mix([n, o])

    if glitch == True:
        if superglitch is not False:
            mlen = dsp.mstf(superglitch)
        else:
            mlen = dsp.flen(out) / 8

        out = dsp.vsplit(out, dsp.mstf(1), mlen)
        out = [dsp.pan(o, dsp.rand()) for o in out]

        if glitchenv == True:
            out = [dsp.env(o) for o in out]

        if glitchpad > 0:
            out = [dsp.pad(o, 0, dsp.mstf(dsp.rand(0, glitchpad))) for o in out]

        out = ''.join(dsp.randshuffle(out))

    if pulse == True:
        plen = dsp.mstf(dsp.rand(100, 1000))
        out = dsp.split(out, plen)
        mpul = len(out) / dsp.randint(4, 8)

        out = [dsp.env(o) * mpul for i, o in enumerate(out) if i % mpul == 0]
        opads = dsp.wavetable('sine', len(out), dsp.rand(plen * 0.25, plen))
        out = [dsp.pad(o, 0, int(opads[i])) for i, o in enumerate(out)]
        out = dsp.env(''.join(out))

    return dsp.play(dsp.amp(out, volume))
