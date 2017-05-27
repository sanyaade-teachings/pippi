import random
from pippi import dsp, wavetables, oscs

# Create a list of random values, padded 
# with zeros on either end to use as the 
# waveform for the wavetable osc
wavetable = [0] + [ random.triangular(-1, 1) for _ in range(20) ] + [0]

# Create an osc with an initial freq of 80hz
osc = oscs.Osc(80, wavetable=wavetable)

# Render 3 seconds of audio
out = osc.play(44100 * 3)

# Write the audio to a soundfile
out.write('simple_custom_wavetable.wav')
