import dsp
import tune

def play(args):
    length = dsp.stf(2)
    volume = 1.0
    w = 50 
    measures =1 
    beats = 8 
    bpm = 72.0

    drums = [{
        'name': 'clap',
        'snd': dsp.read('sounds/clapshake.wav').data,
        'pat': [0, 1, 0, 0],
        'offset': 100,
        }, {
        'name': 'hihat',
        'snd': dsp.read('sounds/hihat.wav').data,
        'pat': [1],
        'offset': 400,
        }, {
        'name': 'snare',
        'snd': dsp.read('sounds/snare.wav').data,
        'pat': [0, 1, 0, 0],
        'offset': 100,
        }, {
        'name': 'kick',
        'snd': dsp.read('sounds/kick.wav').data,
        'pat': [1, 0, 0, 0, 0, 0, 0, 0],
        'offset': 100,
        }]

    wtypes = ['sine', 'phasor', 'line', 'saw']

    for arg in args:
        a = arg.split(':')

        if a[0] == 'v':
            volume = float(a[1]) / 100.0

        if a[0] == 'w':
            w = int(a[1])

        if a[0] == 'm':
            measures = int(a[1])

        if a[0] == 'b':
            beats = int(a[1])

        if a[0] == 'bpm':
            bpm = float(a[1])

    out = ''

    if(w <= 11):
        w = 11

    beats = beats * measures

    beat = dsp.mstf(60000.0 / bpm) / 4
    w = int(beat * (w / 100.0))

    def kickit(snd):
        out = ''
        for b in range(beats):
            s = dsp.cut(snd['snd'], dsp.randint(0, snd['offset']), w)
            s = dsp.pad(s, 0, beat - dsp.flen(s))
            amp = snd['pat'][b % len(snd['pat'])]

            if amp == 1:
                out += s
            else:
                out += dsp.pad('', 0, beat)

        return out

    layers = []
    for drum in drums:
        layers += [ kickit(drum) ]

    out = dsp.mix(layers)

    out = dsp.split(out, beat * 3)
    out = ''.join(dsp.randshuffle(out))

    return dsp.play(dsp.amp(out, volume))
