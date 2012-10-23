import dsp
import tune

def play(params={}):
    length      = params.get('length', dsp.stf(dsp.rand(0.1, 1)))
    volume      = params.get('volume', 20.0)
    volume = volume / 100.0 # TODO: move into param filter
    octave      = params.get('octave', 2) 
    note        = params.get('note', ['d'])
    note = note[0]
    quality     = params.get('quality', tune.major)
    reps        = params.get('repeats', dsp.randint(1, 3))
    glitch      = params.get('glitch', False)
    superglitch = params.get('superglitch', False)
    glitchpad   = params.get('glitch-padding', 0)
    glitchenv   = params.get('glitch-envelope', False)
    pulse       = params.get('pulse', False)
    env         = params.get('envelope', False)
    ratios      = params.get('ratios', tune.terry)
    pad         = params.get('padding', False)
    pulsar      = params.get('pulsar', False)
    harmonics   = params.get('harmonics', False)
    bpm         = params.get('bpm', 75.0)
    arps        = params.get('arps', False)
    width       = params.get('width', False)
    wform       = params.get('waveforms', ['sine', 'line', 'phasor'])
    instrument  = params.get('instrument', 'r')
    scale       = params.get('scale', [1,6,5,4,8])

    if instrument == 'r':
        instrument = 'rhodes'
        tone = dsp.read('sounds/220rhodes.wav').data
    elif instrument == 'c':
        instrument = 'clarinet'
        tone = dsp.read('sounds/clarinet.wav').data
    elif instrument == 'v':
        instrument = 'vibes'
        tone = dsp.read('sounds/cz-vibes.wav').data
    elif instrument == 't':
        instrument = 'tape triangle'
        tone = dsp.read('sounds/tape220.wav').data
    elif instrument == 'g':
        instrument = 'guitar'
        tone = dsp.mix([dsp.read('sounds/guitar.wav').data, 
                        dsp.read('sounds/banjo.wav').data])


        #if a[0] == 'w':
            #if param.istype(a[1], 'b'):
                #width = dsp.bpm2ms(bpm) / param.convert(a[1])
                #width = dsp.mstf(width)

            #elif param.istype(a[1], 'ms'):
                #width = dsp.mstf(param.convert(a[1]))

            #else:
                #width = param.convert(a[1])

        #if a[0] == 't':
            #if param.istype(a[1], 'b'):
                #length = dsp.bpm2ms(bpm) / param.convert(a[1])
                #length = dsp.mstf(length)
            #elif param.istype(a[1], 'ms'):
                #length = dsp.mstf(param.convert(a[1]))
            #elif param.istype(a[1], 's'):
                #length = dsp.stf(param.convert(a[1]))
            #else:
                #length = param.convert(a[1])

        #if a[0] == 'pp':
            #pulsar = True

    wtypes = ['sine', 'gauss']

    out = ''
    for i in range(reps):
        scale = dsp.randshuffle(scale)
        freq = tune.step(i, note, octave, scale, quality, ratios)

        if instrument == 'clarinet':
            diff = freq / 293.7
        else:
            diff = freq / 440.0

        n = dsp.transpose(tone, diff)

        if env is not False:
            n = dsp.env(n, env)

        #if arps is not False:
            #length = dsp.mstf(((60000.0 / bpm) / 2) / reps)

        n = dsp.fill(n, length)

        if width is not False:
            #if isinstance(width, float):
                #width = int(length * width)

            n = dsp.pad(dsp.cut(n, 0, width), 0, length - width)

        if harmonics:
            o = [dsp.tone(length, freq * i * 0.5) for i in range(4)]
            o = [dsp.env(oo) for oo in o]
            o = [dsp.pan(oo, dsp.rand()) for oo in o]

            if instrument == 'clarinet':
                olow = 0.3
                ohigh = 0.8
            else:
                olow = 0.1
                ohigh = 0.5

            o = dsp.mix([dsp.amp(oo, dsp.rand(olow, ohigh)) for oo in o])

            o = dsp.mix([n, o])
        else:
            o = n

        if env is not False:
            o = dsp.env(o, env)

        if pad is not False:
            o = dsp.pad(o, 0, pad)

        out += o

    if glitch == True:
        if superglitch is not False:
            mlen = dsp.mstf(superglitch)
        else:
            mlen = dsp.flen(out) / 8

        out = dsp.vsplit(out, dsp.mstf(1), mlen)
        out = [dsp.pan(o, dsp.rand()) for o in out]

        if glitchenv is not False:
            out = [dsp.env(o, glitchenv) for o in out]

        if glitchpad > 0:
            out = [dsp.pad(o, 0, dsp.mstf(dsp.rand(0, glitchpad))) for o in out]

        out = ''.join(dsp.randshuffle(out))

    if pulse == True:
        plen = dsp.mstf(dsp.rand(500, 1200))
        out = dsp.split(out, plen)
        mpul = len(out) / dsp.randint(4, 8)

        out = [dsp.env(o) * mpul for i, o in enumerate(out) if i % mpul == 0]
        opads = dsp.wavetable('sine', len(out), dsp.rand(plen * 0.25, plen))
        out = [dsp.pad(o, 0, int(opads[i])) for i, o in enumerate(out)]
        out = dsp.env(''.join(out))

    if pulsar == True:
        out = dsp.split(out, 441)
        freqs = dsp.wavetable('sine', len(out), 1.01, 0.99)
        out = [ dsp.transpose(out[i], freqs[i]) for i in range(len(out)) ]
        out = ''.join(out)

    return dsp.amp(out, volume)
