# Pippi
## Computer music with python

Still pretty alpha!

### Install last published version from pip:
    pip install pippi

### Install latest version from source:
    
    python setup.py install

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

[More documentation at Read The Docs.](http://pippi.readthedocs.org)

