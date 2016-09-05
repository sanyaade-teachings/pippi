import dsp

#################
# Unit conversion
#################

def stf(s, r=None):
    if r is not None:
        s = dsp.rand(s, r)

    ms = s * 1000.0
    return mstf(ms)

def mstf(ms, r=None):
    if r is not None:
        ms = dsp.rand(ms, r)

    frames_in_ms = dsp.SAMPLING_RATE / 1000.0
    frames = ms * frames_in_ms

    return int(frames)

def ftms(frames):
    ms = frames / float(dsp.SAMPLING_RATE) 
    return ms * 1000

def fts(frames):
    s = frames / float(dsp.SAMPLING_RATE)
    return s

def ftc(frames):
    frames = int(frames)
    frames *= dsp.BIT_WIDTH
    frames *= dsp.CHANNELS

    return frames

def fth(frames):
    """ frames to hz """
    if frames > 0:
        return (1.0 / frames) * dsp.SAMPLING_RATE

    return 0

def htf(hz):
    """ hz to frames """
    if hz > 0:
        frames = dsp.SAMPLING_RATE / float(hz)
    else:
        frames = 1 # 0hz is okay...

    return int(frames)

def bpm2ms(bpm):
    return 60000.0 / float(bpm)

def bpm2frames(bpm):
    return int((bpm2ms(bpm) / 1000.0) * dsp.SAMPLING_RATE) 


