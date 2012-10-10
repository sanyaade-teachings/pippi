import dsp
import tune

def play(args):
    length = dsp.stf(20)
    volume = 0.5 
    octave = 3
    note = 'd'
    quality = tune.major
    m = 1
    width = 0
    waveform = 'vary'
    chirp = False

    harmonics = [1,2]
    scale = [1,4,6,5,8]
    wtypes = ['sine', 'phasor', 'line', 'saw']
    ratios = tune.terry

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

        if a[0] == 'm':
            m = float(a[1])

        if a[0] == 'w':
            width = dsp.mstf(float(a[1]))

        if a[0] == 'q':
            if a[1] == 'M':
                quality = tune.major
            elif a[1] == 'm':
                quality = tune.minor
            else:
                quality = tune.major

        if a[0] == 'h':
            harmonics = [int(s) for s in a[1].split('.')]

        if a[0] == 'g':
            glitch = True

        if a[0] == 'wf':
            waveform = a[1]

        if a[0] == 'tr':
            ratios = getattr(tune, a[1], tune.terry)


    def chirp(s):
        length = dsp.flen(s)

        #chirps = [ dsp.chirp(dsp.randint(10, 10000), 60, 5000, dsp.randint(1,100)) for c in range(100) ]
        chirps = [ dsp.chirp(
                        numcycles=dsp.randint(50, 1000), 
                        lfreq=dsp.rand(9000, 12000), 
                        hfreq=dsp.rand(14000, 20000), 
                        length=441 + (i * 41),
                        etype=dsp.randchoose(['gauss', 'sine', 'line', 'phasor']),
                        wform=dsp.randchoose(['sine', 'tri', 'phasor', 'line'])) 
                    for i in range(100) ]
        chirps = [ dsp.pan(c, dsp.rand()) for c in chirps ]

        chirps = ''.join(chirps)

        return dsp.fill(chirps, length)



    tones = []
    m *= 1.0
    for i in range(dsp.randint(2,4)):
        freq = tune.step(i, note, octave, dsp.randshuffle(scale), quality, ratios)

        snds = [ dsp.tone(length, freq * h, waveform) for h in harmonics ]
        for snd in snds:
            snd = dsp.vsplit(snd, dsp.mstf(10 * m), dsp.mstf(100 * m))
            if width != 0:
                for ii, s in enumerate(snd):
                    if width > dsp.mstf(5):
                        owidth = int(width * dsp.rand(0.5, 2.0))
                    else:
                        owidth = width

                    olen = dsp.flen(s)
                    s = dsp.cut(s, 0, owidth)
                    s = dsp.pad(s, 0, olen - dsp.flen(s)) 
                    snd[ii] = s

            snd = [ dsp.env(s, dsp.randchoose(wtypes)) for s in snd ]
            snd = [ dsp.pan(s, dsp.rand()) for s in snd ]
            snd = [ dsp.amp(s, dsp.rand()) for s in snd ]
                
            if chirp == True:
                snd = [ chirp(s) for s in snd ]

            snd = ''.join(snd)

            tones += [ snd ]

    out = dsp.mix(tones)
    out = dsp.pan(out, 0)

    return dsp.amp(out, volume)
