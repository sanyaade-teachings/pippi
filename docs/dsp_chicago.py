##
# A few pippi examples for the DSP Chicago meetup.
##

# The dsp module is the core pippi module and contains a set of 
# functions for sound manipulation, list operations, unit conversion,
# file IO, and other general utility functions.
from pippi import dsp

# The tune module includes a handful of built in tuning systems and 
# some utility functions for pitch calculation and translation.
from pippi import tune

##
# Basic synthesis
##

"""
Pippi has a handful of waveforms built in to choose from, including:
    - sine
    - cosine
    - hanning
    - triangle
    - sawtooth
    - phasor
    - variable (sinewave randomly modulated by noise)
    - impulse
    - square

    The code below generates 20 tones at 330hz, between 
    500 and 2000 milliseconds in length. It randomly chooses 
    a waveform to use from the above list for each tone.
"""

# Our list of waveforms to choose from
waveform_types = [
        'sine',
        'cos',
        'hann',
        'tri',
        'saw',
        'phasor',
        'vary',
        'impulse',
        'square',
        ]

# An empty list to store the generated tones
short_tones = []

# Loop through 20 times and append a new tone to the list
for i in range(20):
    # The mstf() function converts milliseconds to 
    # frames at the standard sampling rate.
    #
    # Pippi has its own set of convenience wrappers for 
    # random number generation. By default, the standard 
    # python random module is used. When a seed is passed, 
    # a custom function is used. It is also possible to inject 
    # your own random function. 
    # 
    # rand() returns a float between 
    # the specified bounds. rand(low, high)
    tone_length = dsp.mstf(dsp.rand(500, 2000))

    # Randomly choose a waveform from our list
    waveform_type = dsp.randchoose(waveform_types)

    # Generate a single short tone at 220hz
    short_tone = dsp.tone(tone_length, 220, waveform_type)

    # Add that tone to our list of short tones
    short_tones.append(short_tone)

# Lets pause here and render out what we've got so far.
# To render all the tones as a single sound file, we've got to 
# first join our list together into a single sound.
output = ''.join(short_tones)

# Now we can use pippi's write() function to render this sound to disk.
# The second argument is the filename to write to. A .wav extension will
# automatically be added. (Pippi can only read and write WAV data for now!)
dsp.write(output, 'short_tones')

# Next, lets create a simple function to randomly pan each tone to a different 
# position in the stereo field, and apply a randomly selected amplitude envelope
# to each tone.
def pan_and_envelope(short_tones):
    # A few of the possible envelope types the env() function may use
    envelope_types = ['hann', 'sine', 'line', 'phasor']

    # Loop through each tone and randomly pan it. pan() takes a sound as its first 
    # argument and a float between 0 and 1 as the second. 0 is hard left, 1 is hard right.
    # By default, rand() will return a float between 0 and 1 when no arguments are passed.
    short_tones = [ dsp.pan(short_tone, dsp.rand()) for short_tone in short_tones ]

    # Loop through each (now panned) tone and apply a randomly selected amplitude envelope
    # from the above set of possible envelope types.
    short_tones = [ dsp.env(short_tone, dsp.randchoose(envelope_types)) for short_tone in short_tones ]

    return short_tones

# Take our short_tones and process them, storing the result in a new list: short_tones_pan_envelope
short_tones_pan_envelope = pan_and_envelope(short_tones)

# Join the new tones so we can write the whole thing to disk. And store it in 'output'
output = ''.join(short_tones_pan_envelope)

# Write output to disk
dsp.write(output, 'short_tones_pan_envelope')

# Pitch shifted layers
speeds = [ 1.0, 1.5, 2.0, 1.33, 2.5, 3.0, 4.0, 8.0, 16.0, 32.0 ]
layers = []
for layer in range(6):
    layer = dsp.randshuffle(short_tones_pan_envelope)
    layer = [ dsp.transpose(tone, dsp.randchoose(speeds)) for tone in layer ]
    layer = ''.join(layer)
    layers += [ layer ]

output = dsp.mix(layers)
dsp.write(output, 'short_tones_layers')

degrees = [ 1, 3, 5, 8, 9, 12 ]
pitches = tune.fromdegrees(degrees, octave=3, root='a', ratios=tune.terry)

# Some more FX
for layer_index, layer in enumerate(layers):
    layer = dsp.vsplit(layer, dsp.mstf(1), dsp.mstf(300))
    layer = dsp.randshuffle(layer)
    for i, segment in enumerate(layer):
        if dsp.randint(0,3) == 0:
            layer[i] = dsp.pine(segment, dsp.flen(segment) * dsp.randint(1, 10), dsp.randchoose(pitches))

        if dsp.randint(0,2) == 0:
            layer[i] = dsp.alias(segment)

    layer = ''.join(layer)

    # pulses
    layer = dsp.split(layer, dsp.mstf(100))

    pulses = dsp.breakpoint( [ dsp.rand() for r in range(4) ], len(layer))
    pulses = [ dsp.mstf(70 * (p * 0.2 + 1)) for p in pulses ]

    layer = [ dsp.env(segment, 'phasor') for segment in layer ]
    layer = [ dsp.pad(segment, 0, pulses[si]) for si, segment in enumerate(layer) ]
    layer = ''.join(layer)

    layers[layer_index] = layer

output = dsp.mix(layers)
dsp.write(output, 'short_tones_fx')


