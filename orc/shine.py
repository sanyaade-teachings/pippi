from pippi import dsp
from pippi import tune

def play(params={}):
    length      = params.get('length', dsp.stf(dsp.rand(0.1, 1)))
    volume      = params.get('volume', 20.0)
    volume = volume / 100.0 # TODO: move into param filter
    octave      = params.get('octave', 2) + 1 # Add one to compensate for an old error for now
    note        = params.get('note', ['db'])
    note = note[0]
    quality     = params.get('quality', tune.major)
    glitch      = params.get('glitch', False)
    superglitch = params.get('superglitch', False)
    glitchpad   = params.get('glitch-padding', 0)
    glitchenv   = params.get('glitch-envelope', False)
    env         = params.get('envelope', False)
    ratios      = params.get('ratios', tune.terry)
    pad         = params.get('padding', False)
    bend        = params.get('bend', False)
    bpm         = params.get('bpm', 75.0)
    width       = params.get('width', False)
    wform       = params.get('waveforms', ['sine', 'line', 'phasor'])
    instrument  = params.get('instrument', 'r')
    scale       = params.get('scale', [1,6,5,4,8])
    shuffle     = params.get('shuffle', False) # Reorganize input scale
    reps        = params.get('repeats', len(scale))
    alias       = params.get('alias', False)
    phase       = params.get('phase', False)

    if instrument == 'r':
        instrument = 'rhodes'
        tone = dsp.read('sounds/220rhodes.wav').data
    elif instrument == 'c':
        instrument = 'clarinet'
        tone = dsp.read('sounds/clarinet.wav').data
    elif instrument == 'v':
        instrument = 'vibes'
        tone = dsp.read('sounds/glock220.wav').data
    elif instrument == 't':
        instrument = 'tape triangle'
        tone = dsp.read('sounds/tape220.wav').data
    elif instrument == 'g':
        instrument = 'glade'
        tone = dsp.read('sounds/glade.wav').data 
    elif instrument == 'i':
        instrument = 'input'
        tone = dsp.capture(dsp.stf(1))


    out = ''

    # Translate the list of scale degrees into a list of frequencies
    freqs = tune.fromdegrees(scale, octave, note, quality, ratios)

    if shuffle is not False:
        freqs = dsp.randshuffle(freqs)

    if phase is not False:
        ldivs = [0.5, 0.75, 2, 3, 4]
        ldiv = dsp.randchoose(ldivs)
        length = dsp.bpm2ms(bpm) / ldiv
        length = dsp.mstf(length)
        reps = ldiv if ldiv > 1 else 4

    for i in range(reps):
        #freq = tune.step(i, note, octave, scale, quality, ratios)
        freq = freqs[i % len(freqs)]

        if instrument == 'clarinet':
            diff = freq / 293.7
        elif instrument == 'vibes':
            diff = freq / 740.0 
        else:
            diff = freq / 440.0

        clang = dsp.transpose(tone, diff)

        if env is not False:
            clang = dsp.env(clang, env)

        clang = dsp.fill(clang, length)

        if instrument == 'vibes':
            clang = dsp.pad(clang, 0, length - dsp.flen(clang))
        else: 
            clang = dsp.fill(clang, length)

        if width is not False:
            width = int(length * float(width))

            clang = dsp.pad(dsp.cut(clang, 0, width), 0, length - width)

        if env is not False:
            clang = dsp.env(clang, env)

        if pad is not False:
            clang = dsp.pad(clang, 0, pad)

        out += clang

    if alias is not False:
        out = dsp.alias(out)

    if glitch is not False:
        mlen = dsp.flen(out) / 8

        out = dsp.vsplit(out, dsp.mstf(5), dsp.mstf(300))
        out = [dsp.pan(o, dsp.rand()) for o in out]

        out = [dsp.env(o, 'sine') for o in out]
    
        out = [dsp.pad(o, 0, dsp.mstf(dsp.rand(0, glitchpad))) for o in out]

        out = ''.join(dsp.randshuffle(out))

    if bend == True:
        out = dsp.split(out, 441)
        freqs = dsp.wavetable('sine', len(out), 1.01, 0.99)
        out = [ dsp.transpose(out[i], freqs[i]) for i in range(len(out)) ]
        out = ''.join(out)

    return dsp.amp(out, volume)
