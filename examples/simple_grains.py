import cProfile

from pippi import dsp

def render():
    # Load some boinging into a
    # SoundBuffer instance.
    snd = dsp.read('sounds/boing.wav')

    # Returns an empty SoundBuffer instance that 
    # we can dub our grains into.
    out = dsp.silence() 

    # Make several passes over the sound, 
    # layering grains into the buffer
    num_passes = 4

    for i in range(num_passes):
        # Split the sound into fixed sized grains 
        # 400 frames long and loop over them to 
        # process and dub into the output buffer

        num_frames = 400
        recordhead = 0
        for grain in snd.grains(num_frames):
            # Apply a sine window to the grain and attenuate 
            # to 25% of original amplitude
            grain = grain.env('sine') * 0.25

            # Dub the grain into the current recordhead postion
            out.dub(grain, pos=recordhead)

            # Move the recordhead forward
            recordhead += i * 100 + 100

    # Write the output buffer to a WAV file
    out.write('simple_grains.wav')

cProfile.run('render()', sort='time') 
