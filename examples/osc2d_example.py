import random
import time
from pippi import dsp, oscs, tune, wavetables

start_time = time.time()

tlength = 600
out = dsp.buffer(length=tlength)
pos = 0
count = 0
count2 = 0

def make_note(freq, amp, length):
    numtables = random.randint(1, random.randint(5, 20))
    lfo = wavetables.randline(random.randint(10, random.randint(100, 1000)))
    lfo_freq = random.triangular(0.01, 30)

    mod = wavetables.randline(random.randint(10, 100))
    mod_freq = random.triangular(0.001, 2)
    if random.random() > 0.75:
        mod_range = random.triangular(0, random.triangular(0.05, 1))
    else:
        mod_range = random.triangular(0, 0.03)

    pulsewidth = random.random()

    stack = []
    for _ in range(numtables):
        if random.random() > 0.5:
            stack += [ random.choice([dsp.SINE, dsp.SQUARE, dsp.TRI, dsp.SAW]) ]
        else:
            stack += [ wavetables.randline(random.randint(3, random.randint(10, 200))) ]

    return oscs.Osc(stack=stack, window=dsp.SINE, mod=mod, lfo=lfo, pulsewidth=pulsewidth, mod_freq=mod_freq, mod_range=mod_range, lfo_freq=lfo_freq).play(length=length, freq=freq, amp=amp)


def get_freqs(count):
    chords = ['II', 'V9', 'vii*', 'IV7']
    return tune.chord(chords[count % len(chords)], octave=1)

freqs = get_freqs(0)

while pos < tlength:
    print('Swell %s, pos %s' % (count, pos))

    length = random.triangular(0.04, 40)
    freqs = get_freqs(count // 8)
    params = []
    reps = random.randint(20, 30)

    for _ in range(reps):
        freq = freqs[count2 % len(freqs)] * 2**random.randint(0, 4)
        amp = random.triangular(0.001, 0.02)
        params += [ (freq, amp, length * random.triangular(0.75, 1.5)) ]
        count2 += 1

    notes = dsp.pool(make_note, random.randint(5,10), params)

    for note in notes:
        note = note.adsr(a=0.01, d=0.1, s=0.5, r=0.1) * wavetables.randline(random.randint(10, 100))
        out.dub(note, pos + random.triangular(0, 0.1))

    count += 1
    count2 += 1
    pos += random.triangular(length * 0.75, length * 1.25)

out.write('osc2d_out.wav')

elapsed_time = time.time() - start_time
print('Render time: %s seconds' % round(elapsed_time, 2))
print('Output length: %s seconds' % out.dur)
