# Pippi: Computer music with python

This is pre-release software. I am working toward a stable 1.0 and would very much appreciate bug reports, feedback and pull requests!

## Installation

> Note: pippi is currently only compatible with Python 2.x

Pippi was built to run on Linux, OSX and Windows. Linux is fully supported. OSX support is pretty good 
and getting better. Windows is almost completely untested -- seeking beta testers!

### MIDI Support

To enable MIDI support, install the `portmidi` package on your system. (Available via homebrew on OSX)

### C extension support

In order to build the C extensions that pippi uses for most DSP, you will need to install the Python 
development headers. On some systems (like arch linux) this will already be installed.

On debian (including ubuntu & raspbian) systems:

    apt-get install python-dev

On OSX you may need to install xcode to build the C extensions: http://docs.python-guide.org/en/latest/starting/install/osx/

### To install latest version from source:

Make sure you have `pip` installed on your system, since pippi uses the version of `setuptools` distributed with pip for installation.

To use the interactive features you will need to install `pyalsaaudio` (for ALSA support on linux) or `pyaudio` for experimental cross-platform audio support on OSX and Windows.

You may need to install the `libasound2-dev` package on debian systems with `apt-get` before you will be able to install `pyalsaaudio`.

If you're trying the experimental `pyaudio` support, you may need to install `portaudio` on your system first.

When you have the dependencies you selected installed, from the root of the pippi repository run:
    
    python setup.py install


## Now What?

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

### Interactive example

Pippi can use scripts as "generators" for interactive playback and performance.

Here's a simple example generator based on the previous example:

    from pippi import dsp, tune

    def play(ctl):
        key = ctl.get('param').get('key', 'c')
        freqs = tune.fromdegrees([1,3,5,8], octave=3, root=key)
        freq = dsp.randchoose(freqs)
        length = dsp.stf(2, 5) # between 2 & 5 seconds
        waveshape = dsp.randchoose(['sine', 'tri', 'square'])
        amp = dsp.rand(0.1, 0.2)

        out = dsp.tone(length, freq, waveshape, amp)
        out = dsp.env(out, 'phasor')
        out = dsp.taper(out, 20)

        return out


Save it as example.py and from the same directory type `pippi` to enter the interactive console.

To play three voices of your example.py generator type:

    ^_- play example 3

To stop all voices:

    ^_- stop

To have pippi reload the script to allow you to alter the code and hear the changes as it plays, type:

    ^_- reload on

To change the current key (which is just an arbitrary parameter set in the pippi console session) from C major to Eb major:

    ^_- key eb


More commands and help available by typing `help` at the console

[More documentation at Read The Docs.](http://pippi.readthedocs.org)

