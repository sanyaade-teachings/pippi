# Pippi
## Computer music with python

Quite alpha!

### Install with pip:
    
    pip install pippi

Take a look at the generator scripts in orc/ for some examples of usage. [Docs coming...]

### Now What?

Here's a simple 'Hello World!' style example you can try at the python console:

    >>> from pippi import dsp
    >>> out = dsp.tone(dsp.stf(5), freq=220, amp=0.2)
    >>> out = dsp.env(out, 'hann')
    >>> dsp.write(out, 'hello')
    'hello.wav'

Here's a slightly more interesting example:

    >>> from pippi import dsp
    >>> out = [ dsp.tone(dsp.stf(5), 55 * i, 'sine2pi', 0.2) for i in range(1, 5) ]
    >>> out = [ dsp.env(o, dsp.randchoose(['hann', 'tri', 'sine'])) for o in out ]
    >>> out = dsp.mix(out)
    >>> dsp.write(out, 'helloagain')
    'helloagain.wav'

[Here's another more complex (and musical) example.](docs/example.md)

## 'Realtime' performance layer 
(linux-only - requires ALSA)

### Install with pip: 

    pip install pippi[realtime]

The pippi console:

    pippi

    Pippi Console
    pippi: dr o:2 wf:impulse n:d.a.e t:30s h:1.2.3

Starts the pippi console and generates a 30 second long stack of impulse trains on D, A, and E each with partials 1, 2, and 3.

Take a look at the scripts in orc/ for more arguments and generators.
