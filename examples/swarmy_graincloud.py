from pippi import dsp, grains, interpolation, wavetables
import random
import os
import time

PATH = os.path.dirname(os.path.realpath(__file__))
print(__file__)
start_time = time.time()

def makecloud():
    snd = dsp.read('%s/sounds/linus.wav' % PATH)
    grainlength = wavetables.randline(random.randint(50, 500), lowvalue=0.001, highvalue=random.triangular(0.02, 0.1))
    grid = grainlength * 0.5
    speed = wavetables.randline(random.randint(10, 100), lowvalue=0.25, highvalue=random.triangular(0.25, 10))
    amp = wavetables.randline(random.randint(10, 500), lowvalue=0.125, highvalue=random.triangular(0.5, 1))

    return grains.Cloud(snd, 
                window=dsp.HANN, 
                position=dsp.PHASOR, 
                speed=speed, 
                grid=grid, 
                amp=amp,
                grainlength=grainlength, 
                spread=random.random(),
                jitter=random.triangular(0, 0.1),
            ).play(30)

numclouds = 30
out = dsp.buffer(length=30)
for i in range(numclouds):
    cloud = makecloud()
    out.dub(cloud, 0)

out.write('%s/swarmy_graincloud.wav' % PATH)
elapsed_time = time.time() - start_time
print('Render time: %s seconds' % round(elapsed_time, 2))
print('Output length: %s seconds' % round(out.dur, 2))
