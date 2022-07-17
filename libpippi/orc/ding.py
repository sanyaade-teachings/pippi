from pippi import dsp, fx, oscs

LOOP = True

def play(ctx):
    ctx.log('foo')
    for _ in range(3):
        adc = ctx.adc(dsp.rand(4, 10), offset=dsp.rand(0, 1)) 
        #adc = fx.hpf(adc, dsp.rand(100, 5000))
        #adc = fx.crush(adc, bitdepth=dsp.rand(8000, 16000), samplerate=dsp.rand(8000, 48000))
        #adc = fx.fold(adc, dsp.rand(5, 10))
        adc = fx.norm(adc, dsp.rand(0.5, 0.85))
        #kkc = fx.bpf(adc, 500)
        adc = adc.speed(shapes('sine', 0.5, 3))
        #adc = adc.env('hann')
        #out = adc.taper(0.05).pan(dsp.rand())
        out = adc.pan(dsp.rand())
        #out = out * dsp.win('pluckout').taper(dsp.MS*30).repeated(dsp.randint(10,100))
        yield out * 0.3
