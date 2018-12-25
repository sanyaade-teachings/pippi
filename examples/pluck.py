from pippi import dsp, pluck, tune, fx

freqs = tune.chord('I11', octave=2)

length = 60
out = dsp.buffer(length=length)

pos = 0
count = 0
while pos < length:
    p = pluck.PluckedString(
            freq=freqs[count % len(freqs)] * 2**dsp.randint(-1,4), 
            pick=dsp.rand(0.1, 0.9), 
            #seed=[0, 0.3, 0.1, 0.35, 0.05, 0.35, 0.1, 0], 
            seed=dsp.wt(dsp.SQUARE) + dsp.wt(dsp.SQUARE),
            pickup=dsp.rand(0.1, 0.9), 
            amp=0.35).play(dsp.rand(0.5, 1)).taper(dsp.MS*2)

    out.dub(p.pan(dsp.rand()), pos)

    pos += 0.25
    count += 1

out = fx.norm(out, 0.8)

out.write('pluck.wav')

