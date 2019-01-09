## TUTORIAL 001 - SoundBuffers & basic operations
##
## This tutorial introduces one of the core elements in `pippi`: 
## the `SoundBuffer` class. 

# The `dsp` module in pippi is basically a shortcut that provides 
# many easy initialization helpers -- in this case we'll use it to 
# read a WAV file from disk into memory as a SoundBuffer for further 
# manipulation.
from pippi import dsp

# Assuming you run this script from within the `tutorials` directory, 
# here we're loading a 10 second long stereo WAV file recording of an 
# electric guitar which lives in the `tests/sounds` directory.
guitar = dsp.read('../tests/sounds/guitar10s.wav')

# Print the type, length in frames, and duration in seconds
print('I am a %s -- %s frames and %.2f seconds long' % (type(guitar), len(guitar), guitar.dur))

# Audio file I/O is done with the libsndfile library which supports 
# OGG, FLAC and other compressed soundfile types as well as standard 
# uncompressed PCM audio types. Lets save a copy of this guitar sound 
# as a FLAC in the current directory by calling the `write` method available 
# to any SoundBuffer.
guitar.write('guitar-unaltered.flac')

# Many sound transformations are available directly as methods on the SoundBuffer.
# Lets slow the guitar down to half-speed, print info about it again and then save the 
# result as a new file.
slow_guitar = guitar.speed(0.5)

# Print the updated information
print('I am a slower %s -- %s frames and %.2f seconds long' % (type(slow_guitar), len(slow_guitar), slow_guitar.dur))

# Save a copy -- this time as a WAV file
slow_guitar.write('guitar-slow.wav')

# We can mix the sounds together and save the result using the mix (&) operator
mixed_guitars = slow_guitar & guitar
mixed_guitars.write('guitar-mixed.wav')

print('I am a mixed %s -- %s frames and %.2f seconds long' % (type(mixed_guitars), len(mixed_guitars), mixed_guitars.dur))

# Often it's useful to mix many processed segments into a final output buffer. Lets use the `dsp.buffer` shortcut to create a 
# new empty SoundBuffer.
out = dsp.buffer()

# The `tune` module has many helper functions for working with musical pitches and tuning systems.
from pippi import tune

# Lets make a list of frequencies that represent a major triad in the key of C, starting at C3 and using a just tuning.
freqs = tune.chord('I', key='C', octave=3, ratios=tune.just)

# These are the frequencies
print('Cmaj: %s' % freqs)

# In order to use our `speed` method to pitch shift the guitar and make a C major chord, 
# we need to convert these absolute frequences into relative speeds. The original guitar 
# note is an A at roughly 220hz, so the speeds can be derived by using this value.

# A220 is A2 in scientific pitch notation -- we can also use a helper to get the frequency 
# from the note name:
original_freq = tune.ntf('A2')

# Now we can make a new list of relative speeds by dividing the original frequency into the target frequency
speeds = [ new_freq / original_freq for new_freq in freqs ]

# Lets dub a copy of the guitar note at each of these speeds into our output buffer every 1.5 seconds.
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
