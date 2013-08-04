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

    # Generate a single short tone at 330hz
    short_tone = dsp.tone(tone_length, 330, waveform_type)

    # Add that tone to our list of short tones
    short_tones.append(short_tone)

# Lets pause here and render out what we've got so far.
# To render all the tones as a single sound file, we've got to 
# first join our list together into a single sound.
short_tones_output = ''.join(short_tones)

# Now we can use pippi's write() function to render this sound to disk.
# The second argument is the filename to write to. A .wav extension will
# automatically be added. (Pippi can only read and write WAV data for now!)
dsp.write(short_tones_output, 'short_tones')

