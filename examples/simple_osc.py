import random

from pippi import dsp
from pippi.oscs import Osc
import os

PATH = os.path.dirname(os.path.realpath(__file__))
print(__file__)

# Create a wavetable osc with a randomly 
# selected waveform type
osc = Osc(freq=440, wavetable=dsp.RND)

# Length in seconds
length = 1

# Fill a SoundBuffer with output from the osc
out = osc.play(length) * 0.5

# Write the output buffer to a WAV file
out.write('%s/simple_osc.wav' % PATH)
