from pippi import dsp, oscs, tune, fx, noise
import time

groups = (5,6,7)
#dsp.seed(time.time())

def play(ctx):
    cc1 = ctx.p.cc1 or 0.1
    cc2 = ctx.p.cc2 or 0.1
    cc3 = ctx.p.cc3 or 0.1
    cc4 = ctx.p.cc4 or 0.1
    cc5 = ctx.p.cc5 or 0.1

    #for _ in range(dsp.randint(4,10)):
    for _ in range(1):
        freqs = tune.degrees([1,2,3,4,5,6,7], key='c', octave=2)
        l = cc1 * dsp.rand(0.9, 1.1) * 3 + 0.001

        #out = oscs.Pulsar2d(
        #    freq=dsp.choice(freqs),
        #    pulsewidth=dsp.rand(0.1, 1),
        #).play(l).env('rnd')
        lf = 8000 * cc2
        hf = dsp.rand(9000, 12000) * cc2
        out = noise.bln('sine', l, lf, hf).env('pluckout')

        #out = fx.crush(out)
        adc = ctx.adc(out.dur) * 10

        out.dub(adc)
        out = fx.lpf(out, 500)

        #out = out.env('hann')
        yield out * cc3
