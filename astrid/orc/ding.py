from pippi import dsp
from pippi import oscs
from pippi import tune
from pippi import fx
from pippi import shapes

# Uncomment this to retrigger after each play
LOOP = True

stack = ['sine', 'hann', 'tri', 'cos', 'square', 'rnd', 'rnd']

# At least one play method is required in instrument scripts
def play(ctx):
    # Set the pulsewidth of the osc, a value between 0 and 1
    # The default pulsewidth is 1 unless it is given a value 
    # with the play message, like:
    #     p demo pw=0.3
    pw = max(0, min(float(ctx.p.pw or 1), 1))

    # Print the pulsewidth value to the log
    #ctx.log('pw: %s' % pw)

    # Generate a list of floats: frequencies to choose from
    # the degrees function turns a list of scale degrees into 
    # a list of frequencies.
    freqs = tune.degrees([dsp.randint(1,11)], key='c', scale=tune.MINOR, octave=dsp.randint(2,5))

    # Render a buffer of output
    out = oscs.Pulsar2d(
            # Set the stack of wavetables for the pulsar osc
            wavetables=stack,
            # choose one of the frequencies
            freq=dsp.choice(freqs), 
            # Attenuate the volume a random amount from 0.125 to 0.25
            amp=dsp.rand(0.0125, 0.5), 
            pulsewidth=shapes.win('tri', 0.1, 1),
        # Render between 0.1 and 2 seconds of output
        ).play(dsp.rand(1, 2))

    if dsp.rand() > 0.5:
        out = out.env(dsp.win('rnd').repeated(dsp.randint(90, 300)))

    if dsp.rand() > 0.5:
        out = fx.fold(out, dsp.rand(1, 3))

    # Apply an envelope to the buffer: rnd is a special 
    # built-in wavetable that randomly selects one of
    # pippi's built-in wavetables.
    out = out.env('hann')

    # Pan the stereo buffer randomly and 
    # add a 10ms taper to smooth out pops
    out = out.pan(dsp.rand()).taper(dsp.MS*10)

    out = fx.lpf(out, 3000)

    # off to the renderer!
    yield out.env('pluckout') * 2
