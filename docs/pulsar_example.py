from pippi import dsp

"""
Pulsar synthesis example.

Okay so this is super boring, but play with it & see!

Lots of different sounds are possible by just changing the 
shape of the wavetables being passed into dsp.pulsar().

Try wrapping the whole thing in a loop & using 'random' as 
your first arg to dsp.wavetable() to get a sense of the range.

You can also give it arbitrarily constructed wavetables as long 
as they are a python list of floats.
"""

waveform = dsp.wavetable('sine2pi', 512)
window = dsp.wavetable('sine', 512)
mod = dsp.wavetable('line', 512)

pulsewidth = 0.5       # 0.0 - 1.0
freq = 220.0           # Hz
amp = 0.5              # 0.0 - 1.0
modFreq = 0.1          # Hz
modRange = 0.5         # Frequency modulation range (scales the mod window. 0 == no modulation)
length = dsp.stf(20)   # Frames (converted here from seconds)

out = dsp.pulsar(freq, length, pulsewidth, waveform, window, mod, modRange, modFreq, amp)

dsp.write(out, 'pulsar_example')
