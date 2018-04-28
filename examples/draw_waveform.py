import random
from pippi import dsp, graph

buflength = 5
out = dsp.buffer(length=buflength)

sounds = [ 
    dsp.read('sounds/snare.wav'), 
    dsp.read('sounds/boing.wav'), 
    dsp.read('sounds/hat.wav'), 
]

numsounds = random.randint(3, 5)

# Put some sounds in a buffer to look at
for _ in range(numsounds):
    sound = random.choice(sounds)
    sound = sound.pan(random.random()) * random.random()

    # Start anywhere between the start and buffer end length minus sound length 
    start = random.triangular(0, buflength-sound.dur)

    out.dub(sound, start)

graph.waveform(out, 'draw_waveform_example.png', width=600, height=300)
