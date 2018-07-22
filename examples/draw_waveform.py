import random
from pippi import dsp, graph
import os

PATH = os.path.dirname(os.path.realpath(__file__))
print(__file__)

buflength = 5
out = dsp.buffer(length=buflength)

sounds = [ 
    dsp.read('%s/sounds/snare.wav' % PATH), 
    dsp.read('%s/sounds/boing.wav' % PATH), 
    dsp.read('%s/sounds/hat.wav' % PATH), 
]

numsounds = random.randint(3, 5)

# Put some sounds in a buffer to look at
for _ in range(numsounds):
    sound = random.choice(sounds)
    sound = sound.pan(random.random()) * random.random()

    # Start anywhere between the start and buffer end length minus sound length 
    start = random.triangular(0, buflength-sound.dur)

    out.dub(sound, start)

graph.waveform(out, '%s/draw_waveform_example.png' % PATH, width=600, height=300)
