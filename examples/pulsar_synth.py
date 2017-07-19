import random
import time
from pippi import dsp, oscs, tune

start_time = time.time()

out = dsp.buffer()
numnotes = 500
freqs = tune.fromdegrees([1,5,8], octave=2, root='g')

for _ in range(numnotes):
    pos = random.randint(0, 44100 * 30)
    length = random.randint(10, 10000)

    wavetable = [0] + [ random.triangular(-1, 1) for _ in range(random.randint(3, 20)) ] + [0]
    mod = [ random.triangular(0, 1) for _ in range(random.randint(3, 20)) ]

    osc = oscs.Osc(wavetable=wavetable, window='sine', mod=mod)
    osc.freq = random.choice(freqs) * 2**random.randint(0, 3)
    osc.mod_freq = random.triangular(0.01, 30)
    osc.mod_range = random.triangular(0, random.choice([0.03, 0.02, 0.01, 3]))
    osc.amp = random.triangular(0.05, 0.1)

    note = osc.play(length)
    note = note.env(random.choice(['sine', 'phasor', 'line']))
    note = note.env('sine')
    note = note.pan(random.random())

    out.dub(note, pos)

out.write('pulsar_synth.wav')
elapsed_time = time.time() - start_time
print('Render time: %s' % round(elapsed_time, 2))
