import dsp
import tune
import math

def play(args):
    length = dsp.stf(20)
    volume = 0.2 
    octave = 2 
    notes = ['d', 'a']
    quality = tune.major
    glitch = False
    waveform = 'sine'
    ratios = tune.terry
    alias = False
    wild = False
    bend = False

    harmonics = [1,2]
    scale = [1,8]
    wtypes = ['sine', 'phasor', 'line', 'saw']

    for arg in args:
        a = arg.split(':')

        if a[0] == 't':
            length = dsp.stf(float(a[1]))

        if a[0] == 'v':
            volume = float(a[1]) / 100.0

        if a[0] == 'o':
            octave = int(a[1])

        if a[0] == 'n':
            notes = a[1].split('.')

        if a[0] == 'q':
            if a[1] == 'M':
                quality = tune.major
            elif a[1] == 'm':
                quality = tune.minor
            else:
                quality = tune.major

        if a[0] == 'tr':
            ratios = getattr(tune, a[1], tune.terry)

        if a[0] == 'h':
            harmonics = [int(s) for s in a[1].split('.')]

        if a[0] == 'g':
            glitch = True

        if a[0] == 'a':
            alias = int(a[1]) if len(a) > 1 else 1

        if a[0] == 'w':
            wild = True

        if a[0] == 'wf':
            waveform = a[1]

        if a[0] == 'bend':
            bend = True

    layers = []
    for note in notes:
        tones = []
        for i in range(dsp.randint(2,4)):
            freq = tune.step(i, note, octave, dsp.randshuffle(scale), quality, ratios)

            snds = [ dsp.tone(length, freq * h, waveform) for h in harmonics ]

            snds = [ dsp.env(s, dsp.randchoose(wtypes), highval=dsp.rand(0.2, 0.4)) for s in snds ]
            snds = [ dsp.pan(s, dsp.rand()) for s in snds ]

            tones += [ dsp.mix(snds) ]

        if bend is not False:
            def bendit(out=''):
                out = dsp.split(out, 441)
                freqs = dsp.wavetable('sine', len(out), dsp.rand(1.0, 1.04), dsp.rand(1.0, 0.96))
                out = [ dsp.transpose(out[i], freqs[i]) for i in range(len(out)) ]
                return ''.join(out)

            tones = [ bendit(tone) for tone in tones ]

        layer = dsp.mix(tones)

        if alias is not False:
            layer = dsp.split(layer, alias)
            layer = [ dsp.pan(l, dsp.rand()) for l in layer ]
            layer = ''.join(layer)

        if wild is not False:
            layer = dsp.vsplit(layer, 41, 4410)
            #layer = [ dsp.drift(l, dsp.rand() + 0.25) for l in layer ]
            #layer = [ dsp.fnoise(l, dsp.rand()) for l in layer ]
            layer = [ dsp.amp(dsp.amp(l, dsp.rand(10, 20)), 0.5) for l in layer ]
            #layer = [ dsp.alias(l) for l in layer ]
            layer = ''.join(layer)

        

        if glitch:
            inlen = dsp.flen(layer) * 0.25
            instart = 0

            midlen = dsp.flen(layer) * 0.5
            midstart = inlen

            endlen = dsp.flen(layer) - (inlen + midlen)
            endstart = inlen + midlen
            
            outin = dsp.cut(layer, instart, inlen)
            outmid = dsp.cut(layer, midstart, midlen)
            outend = dsp.cut(layer, endstart, endlen)

            layer = "%s%s%s" % (dsp.env(outin, 'line'), outmid, dsp.env(outend, 'phasor'))
        else:
            layer = dsp.env(layer, 'gauss')

        layers += [ layer ]

    out = dsp.mix(layers)

    return dsp.play(dsp.amp(out, volume))
