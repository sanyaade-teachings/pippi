
from pippi import dsp

guitar = dsp.read('../tests/sounds/guitar10s.wav')

print('I am a %s -- %s frames and %.2f seconds long' % (type(guitar), len(guitar), guitar.dur))

guitar.write('guitar-unaltered.flac')

slow_guitar = guitar.speed(0.5)

print('I am a slower %s -- %s frames and %.2f seconds long' % (type(slow_guitar), len(slow_guitar), slow_guitar.dur))

slow_guitar.write('guitar-slow.wav')

mixed_guitars = slow_guitar & guitar
mixed_guitars.write('guitar-mixed.wav')

print('I am a mixed %s -- %s frames and %.2f seconds long' % (type(mixed_guitars), len(mixed_guitars), mixed_guitars.dur))

out = dsp.buffer()

from pippi import tune

freqs = tune.chord('I', key='C', octave=3, ratios=tune.just)

print('Cmaj: %s' % freqs)

original_freq = tune.ntf('A2')

speeds = [ new_freq / original_freq for new_freq in freqs ]

pos = 0  
beat = 1.5
for speed in speeds:
    # Create a pitch-shifted copy of the original guitar
    note = guitar.speed(speed)

    # Dub it into the output buffer at the current position in seconds
    out.dub(note, pos)

    # Just for fun, lets also dub another copy 400 milliseconds (0.4 seconds) later that's an octave higher
    note = guitar.speed(speed * 2)
    out.dub(note, pos + 0.4) 

    # Now move the write position forward 1.5 seconds
    pos += beat

# Save this output buffer
out.write('guitar-chord.wav')
