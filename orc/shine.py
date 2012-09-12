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
    pad = False
    pulsar = False

    wform = ['sine', 'line', 'phasor']

    instrument = 'rhodes'
    tone = dsp.read('sounds/220rhodes.wav').data

    #def capture(frames, out=''):
        #import alsaaudio
        #import time
        #i = alsaaudio.PCM(alsaaudio.PCM_CAPTURE, 0, 'T6_pair1')

        #i.setchannels(2)
        #i.setrate(44100)
        #i.setformat(alsaaudio.PCM_FORMAT_S16_LE)
        #i.setperiodsize(160)

        #while frames > 0:
            #frames -= 1
            #l,data = i.read()
            
            #if l:
                #out += dsp.byte_string(int(l))

        ##i.close()
        #return out

    #tone = capture(dsp.mstf(2500))

    #scale = [1,3,5,4]
    scale = [1,6,5,4,8]

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

        if a[0] == 'pp':
            pulsar = True

        if a[0] == 'g':
            glitch = True

            a[1] = a[1].split('.') if len(a) > 1 else a[1]
            superglitch = int(a[1][0]) if len(a) > 1 else 1000
            glitchpad = int(a[1][1]) if len(a) > 1 and len(a[1]) > 1 else 5000
            glitchenv = a[1][2] if len(a) > 1 and len(a[1]) > 2 else False 

        if a[0] == 'e':
            env = a[1] if len(a) > 1 else 'sine'

        if a[0] == 'pad':
            pad = dsp.mstf(int(a[1]))

        if a[0] == 'tr':
            ratios = getattr(tune, a[1], tune.terry)

        if a[0] == 's':
            scale = [int(s) for s in a[1].split('.')]

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

        n = dsp.fill(n, length)

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

        #out = dsp.pulsar(out)
        #layers = []

        #for i in range(3):
            #minlen = dsp.mstf(1)
            #maxlen = dsp.mstf(250)
            #pool = dsp.vsplit(out, minlen, maxlen)
            #layer = []

            #for l in pool:
                #if dsp.randint(0, 3) == 0:
                    #if dsp.randint(0, 3) == 0:
                        #slen = dsp.flen(l)
                        #cmin = 41 
                        #cmax = slen 
                        #clen = dsp.randint(cmin, cmax)
                        #l = dsp.cut(l, 0, clen)
                        #l = l * (slen / dsp.flen(l))

                    #if dsp.randint(0, 3) == 2:
                        #l = dsp.alias(l)
                        

                    #l = dsp.env(l, dsp.randchoose(wtypes), True)
                    #l = dsp.pan(l, dsp.rand())
                    #l = dsp.amp(l, dsp.rand(0.1, 1.0))
                    #layer += [ l ]
                #else:
                    #layer += [ dsp.pad('', 0, dsp.flen(l)) ]

            #layers += [ ''.join(layer) ]

        #out = dsp.mix(layers, True, 6)

    return dsp.play(dsp.amp(out, volume))
