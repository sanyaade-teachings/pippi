import dsp

def stretch(snd, length=None, speed=None, grain_size=20):
    """ Crude granular time stretching and pitch shifting """

    original_length = dsp.flen(snd)

    if speed is not None:
        snd = dsp.transpose(snd, speed)

    current_length = dsp.flen(snd)

    if original_length != current_length or length is not None:
        grain_size = dsp.mstf(grain_size)

        numgrains = length / (grain_size / 2)
        numgrains = numgrains if numgrains > 0 else 1

        block_size = current_length / numgrains

        grains = []
        original_position = 0
        count = 0

        while count <= numgrains:
            grain = dsp.cut(snd, original_position, grain_size)

            grains += [ grain ]

            original_position += block_size
            count += 1

        snd = dsp.cross(grains, dsp.ftms(grain_size / 2))

    return snd

def alias(snd, passthru=False, envelope='flat', split_size=0):
    """
        A simple time domain bitcrush-like effect.

        The sound is cut into blocks between 1 and 64 frames in size if split_size is zero, 
        otherwise split_size is the number of frames in each block.

        Every other block is discarded, and each remaining block is repeated in place.

        Set passthru to True to return the sound without processing. (Can be useful when processing grains in list comprehensions.)

        By default, a random amplitude envelope is also applied to the final sound.
    """
    if passthru:
        return snd

    if envelope == 'flat':
        envelope = False
    elif envelope is None:
        envelope = 'random'

    if split_size == 0:
        split_size = dsp.WINDOW_SIZE / dsp.randint(1, dsp.WINDOW_SIZE)

    packets = dsp.split(snd, split_size)
    packets = [p*2 for i, p in enumerate(packets) if i % 2]

    out = ''.join(packets)

    if envelope:
        out = dsp.env(out, envelope)

    return out 


