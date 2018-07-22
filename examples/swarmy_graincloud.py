from pippi import dsp, grains, interpolation
import random
import os
import time

PATH = os.path.dirname(os.path.realpath(__file__))
print(__file__)
start_time = time.time()

snd = dsp.read('%s/sounds/linus.wav' % PATH)

def makecloud(density):
    print('cloud density', density)
    return grains.GrainCloud(snd * 0.125, 
                win=dsp.HANN, 
                read_lfo=dsp.PHASOR, 
                speed_lfo_wt=interpolation.linear([ random.random() for _ in range(random.randint(10, 1000)) ], 4096), 
                density_lfo_wt=interpolation.linear([ random.random() for _ in range(random.randint(10, 1000)) ], 4096), 
                grainlength_lfo_wt=interpolation.linear([ random.random() for _ in range(random.randint(10, 500)) ], 4096), 
                minspeed=0.25, 
                maxspeed=random.triangular(0.25, 10),
                density=density,
                minlength=1, 
                maxlength=random.triangular(60, 100),
                spread=random.random(),
                jitter=random.triangular(0, 0.1),
            ).play(30)

numclouds = 10
densities = [ (random.triangular(0.1, 2),) for _ in range(numclouds) ]

#clouds = dsp.pool(makecloud, numclouds, densities)
out = dsp.buffer(length=30)
#for cloud in clouds:
for i in range(numclouds):
    cloud = makecloud(*densities[i])
    out.dub(cloud, 0)

out.write('%s/swarmy_graincloud.wav' % PATH)
elapsed_time = time.time() - start_time
print('Render time: %s seconds' % round(elapsed_time, 2))
print('Output length: %s seconds' % round(out.dur, 2))
