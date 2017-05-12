import random

from pippi import dsp

# Load some boinging into a
# SoundBuffer instance.
snd = dsp.read('sounds/boing.wav')

# Returns an empty SoundBuffer instance that 
# we can dub our grains into.
out = dsp.silence() 

# Make several passes over the sound, 
# layering grains into the buffer
num_passes = random.randint(3, 6)

for _ in range(num_passes):
    # Split the sound into fixed sized grains 
    # between 100 and 400 frames long and 
    # loop over them to process and dub into 
    # the output buffer

    fixed_length = random.randint(100, 400)
    recordhead = 0
    for grain in snd.grains(fixed_length):
        # Apply a sine window to the grain and attenuate 
        # by multiplying the buffer with a random amplitude
        amp = random.triangular(0.1, 1)
        grain = grain.env('sine') * amp

        # Dub the grain into the current recordhead postion
        out.dub(grain, pos=recordhead)

        # Move the recordhead to a random position between 
        # 0 and 1,000 frames later than the last position
        recordhead += random.randint(0, 1000)

# Write the output buffer to a WAV file
out.write('simple_grains.wav')

