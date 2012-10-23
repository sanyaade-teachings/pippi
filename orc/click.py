import dsp
import tune

def play(params={}):

    length = params.get('length', dsp.stf(2))
    volume = params.get('volume', 100.0)
    volume = volume / 100.0 # TODO: move into param filter
    width = params.get('width', 50)
    measures = params.get('multiple', 1)
    beats = params.get('repeat', 8)
    bpm = params.get('bpm', 75.0)
    glitch = params.get('glitch', False)
    alias = params.get('alias', False)
    skitter = params.get('skitter', False)
    bend = params.get('bend', True)
    tweet = params.get('tweet', False)
    pattern = params.get('pattern', True)
    playdrums = params.get('d', 'k.h.c')
    playdrums = playdrums.split('.') # TODO: move into param filter

    drums = [{
        'name': 'clap',
        'shortname': 'c',
        'snd': dsp.read('sounds/lowpaperclips.wav').data,
        'pat': [0, 0, 1, 0],
        'vary': [0, 0, 1, 1],
        'offset': 100,
        'width': 0.6,
        'bend': True,
        'alias': False,
        }, {
        'name': 'hihat',
        'shortname': 'h',
        'snd': dsp.read('sounds/paperclips.wav').data,
        'pat': [1],
        'vary': [1],
        'offset': 400,
        'width': 0.1,
        'bend': False,
        'alias': False,
        }, {
        'name': 'snare',
        'shortname': 's',
        'snd': dsp.read('sounds/lowpaperclips.wav').data,
        'pat': [1, 1, 1, 0],
        'vary': [0, 0, 1, 1],
        'offset': 500,
        'width': 0.1,
        'bend': True,
        'alias': True,
        }, {
        'name': 'kick',
        'shortname': 'k',
        'snd': dsp.read('sounds/lowpaperclips2.wav').data,
        'pat': [1, 0, 0, 0, 0, 0, 0, 0],
        'vary': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0],
        'offset': 0,
        'width': 1.0,
        'bend': True,
        'alias': False,
        }]

    wtypes = ['sine', 'phasor', 'line', 'saw']
    #playdrums = ['k', 'h', 'c']

    #for arg in args:
        #a = arg.split(':')

        #if a[0] == 'v':
            #volume = float(a[1]) / 100.0

        #if a[0] == 'width':
            #width = int(a[1])

        #if a[0] == 'm':
            #measures = int(a[1])

        #if a[0] == 'b':
            #beats = int(a[1])

        #if a[0] == 'bpm':
            #bpm = float(a[1])

        #if a[0] == 'd':
            #playdrums = a[1].split('.')

        #if a[0] == 'g':
            #glitch = True

        #if a[0] == 'single':
            #pattern = False

        #if a[0] == 's':
            #skitter = True

        #if a[0] == 'a':
            #alias = True

        #if a[0] == 'be':
            #bend = True

        #if a[0] == 't':
            #tweet = True


    out = ''

    beats = beats * measures

    beat = dsp.mstf(60000.0 / bpm) / 4
    width = int(beat * (width / 100.0))

    def tweeter(o):
        o = dsp.split(o, dsp.randint(3,6))
        oo = dsp.randchoose(o)
        o = [ dsp.env(oo, dsp.randchoose(wtypes)) for h in range(len(o)) ]
        return dsp.env(''.join(o), dsp.randchoose(wtypes))


    def kickit(snd):
        out = ''
        for b in range(beats):
            s = dsp.cut(snd['snd'], dsp.randint(0, snd['offset']), width)

            if snd['alias'] == True:
                s = dsp.alias(s)

            if snd['bend'] == True:
                s = dsp.transpose(s, dsp.rand(-0.4, 0.4) + 1)

            if skitter == True:
                prebeat = dsp.mstf(dsp.randint(10, 400))
            else:
                prebeat = 0

            if tweet == True:
                s = tweeter(s)

            s = dsp.mix([s, dsp.tone(dsp.flen(s), 13000, amp=0.4)])

            s = dsp.pad(s, prebeat, beat - dsp.flen(s))

            if pattern == True:
                if dsp.randint(0,8) == 0:
                    amp = snd['vary'][b % len(snd['vary'])]
                else:
                    amp = snd['pat'][b % len(snd['pat'])]
            else:
                amp = 1.0

            if amp == 1:
                out += s
            elif amp == [1,1]:
                out += dsp.cut(s, 0, dsp.flen(s) / 2) * 2
            elif amp == [0,1]:
                out += dsp.pad(dsp.cut(s, 0, dsp.flen(s) / 2), dsp.flen(s) / 2, 0) 
            elif amp == [1,0]:
                out += dsp.pad(dsp.cut(s, 0, dsp.flen(s) / 2), 0, dsp.flen(s) / 2) 
            else:
                out += dsp.pad('', 0, beat)

        return out

    layers = []
    for drum in drums:
        if drum['shortname'] in playdrums:
            layers += [ kickit(drum) ]

    out = dsp.mix(layers)

    if glitch == True:
        out = dsp.split(out, int(beat * 0.5))
        for i,o in enumerate(out):
            if i % 5 == 0 and dsp.randint(0,1) == 0:
                out[i] = tweeter(o)

        out = ''.join(dsp.randshuffle(out))

    out = dsp.pan(out, 1)

    return dsp.amp(out, volume)
