from pippi import dsp
from pippi import tune
import math

shortname   = 'dr'
name        = 'drone'
device      = 'T6_pair2'
loop        = True

def play(params):
    length = params.get('length', dsp.stf(20))
    volume = params.get('volume', 20.0) 
    volume = volume / 100.0 # TODO move into param filter
    octave = params.get('octave', 2)
    notes = params.get('note', ['c', 'g'])
    quality = params.get('quality', tune.major)
    glitch = params.get('glitch', False)
    waveform = params.get('waveform', 'sine')
    ratios = params.get('ratios', tune.terry)
    alias = params.get('alias', False)
    wild = params.get('wii', False)
    bend = params.get('bend', False)
    env = params.get('envelope', 'gauss')
    harmonics = params.get('harmonics', [1,2])
    scale = params.get('scale', [1,8])
    reps      = params.get('repeats', 1)

    # These are amplitude envelopes for each partial,
    # randomly selected for each. Not to be confused with 
    # the master 'env' param which is the amplit
    wtypes = ['sine', 'phasor', 'line', 'saw']    
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
                freqs = dsp.wavetable('sine', len(out), 1.01, 0.99)
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
            layer = dsp.env(layer, env)

        layers += [ layer ]

    out = dsp.mix(layers) * reps

    return (dsp.amp(out, volume), {'value': {'pitches': [tune.nts(notes[0], octave - 1)] }})
