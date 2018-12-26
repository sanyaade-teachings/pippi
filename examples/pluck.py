from pippi import dsp, oscs, tune, fx
import os
import time

PATH = os.path.dirname(os.path.realpath(__file__))
print(__file__)
start_time = time.time()

freqs = tune.chord('I11', octave=2)

length = 60
out = dsp.buffer(length=length)

pos = 0
count = 0

seed = dsp.wt(dsp.RND) + dsp.wt(dsp.RND)

while pos < length:
    p = oscs.Pluck(
            freq=freqs[count % len(freqs)] * 2**dsp.randint(-1,4), 
            seed=seed,
            pickup=dsp.rand(0, 1), 
            amp=0.35).play(dsp.rand(0.5, 3)).taper(dsp.MS*2)

    out.dub(p.pan(dsp.rand()), pos)

    pos += 0.25
    count += 1

out = fx.norm(out, 0.8)

out.write('%s/pluck.wav' % PATH)

elapsed_time = time.time() - start_time
print('Render time: %s seconds' % round(elapsed_time, 2))
print('Output length: %s seconds' % out.dur)
