from pippi.oscs import Osc

# Sine wavetable osc
osc = Osc(freq=440, wavetable='sine')

# Length in frames
length = 44100

# Fill a SoundBuffer with output from the osc
out = osc.play(length, amp=0.5)

# Write the output buffer to a WAV file
out.write('simple_osc.wav')
