import random
import time
from pippi import dsp, oscs, tune

start_time = time.time()

tlength = 44100 * 150
out = dsp.buffer()
pos = 0
beat = 5000
count = 0

def make_note(freq, lfo_freq, amp, length):
    length = random.randint(length, length * 2)
    numtables = random.randint(1, random.randint(3, 12))
    lfo = random.choice(['sine', 'line', 'tri', 'phasor'])
    wavetables = [ random.choice(['sine', 'square', 'tri', 'saw']) for _ in range(numtables) ]

    osc = oscs.Osc2d(wavetables, lfo)

    print('Note, freq: %s, lfo_freq: %s, amp: %s, length: %s' % (freq, lfo_freq, amp, round(length / 44100, 2)))
    osc.freq = freq * random.choice([0.5, 1])
    osc.lfo_freq = lfo_freq
    osc.amp = amp
    note = osc.play(length)
    note = note.env('random').env('phasor').pan(random.random())

    return note

def get_freqs(count):
    chords = ['II', 'V9', 'vii*', 'IV7']
    return tune.chord(chords[count % len(chords)]) * 2

freqs = get_freqs(0)

while len(out) < tlength:
    print('Swell %s, pos %s' % (count, round(pos/44100, 2)))

    length = random.randint(44100, 44100 * 2)
    freqs = get_freqs(count // 10)
    params = []

    for freq in freqs:
        lfo_freq = random.triangular(0.001, 10)
        amp = random.triangular(0.001, 0.1)
        params += [ (freq, lfo_freq, amp, length) ]

    notes = dsp.pool(make_note, params)

    for note in notes:
        out.dub(note, pos + random.randint(0, 1000))

    count += 1
    pos += beat * random.randint(1, 4) * random.randint(1,3)

out.write('osc2d_out.wav')

elapsed_time = time.time() - start_time
print('Render time: %s seconds' % round(elapsed_time, 2))
print('Output length: %s seconds' % round(len(out)/44100, 2))
