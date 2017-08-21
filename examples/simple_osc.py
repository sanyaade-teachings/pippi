import random

from pippi.oscs import Osc

waveform_types = [
    'square', 
    'sine', 
    'triangle', 
    'saw', 
    'rsaw', 
    'cosine'
]

# Create a wavetable osc with a randomly 
# selected waveform type
osc = Osc(freq=440, wavetable=random.choice(waveform_types))

# Length in frames
length = 44100

# Fill a SoundBuffer with output from the osc
out = osc.play(length) * 0.5

# Write the output buffer to a WAV file
out.write('simple_osc.wav')
