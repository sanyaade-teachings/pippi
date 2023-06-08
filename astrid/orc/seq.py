from pippi import dsp, tune

def trigger(ctx):
    events = []
    freqs = tune.degrees([1,3,6,5,2,4,7,9], octave=3)
    pos = 0
    for i in range(10):
        length = dsp.rand(0.1, 3)
        freq = freqs[i % len(freqs)]
        amp = dsp.rand(0.1, 1)
        events += ctx.t.midi(pos, length, freq, amp)
        pos += dsp.rand(0.1, 1)

    return events
