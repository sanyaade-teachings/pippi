from pippi import dsp, tune

def trigger(ctx):
    midi_events = []
    play_events = []

    freqs = tune.chord('I9')

    pos = 0
    for i in range(dsp.randint(3,10)):
        length = dsp.rand(0.01, 5)
        freq = dsp.choice(freqs) * 2**dsp.randint(0,3)
        amp = dsp.rand(0.5, 1)

        midi_events += ctx.t.midi(pos, length, freq, amp)
        play_events += [ctx.t.play(pos, 'osc', length=length, freq=freq, amp=amp)]

        pos += dsp.rand(0.1, 0.5)

    return events
