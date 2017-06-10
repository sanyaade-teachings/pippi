# Pippi: Computer music with python

v2.0.0 - Alpha 2

## Install dependencies

Pippi requires python 3.5+

You might be lucky and already have it installed! Otherwise, 
the python.org website has installation instructions for your platform: 

    https://www.python.org/downloads/

Right now, `3.6.1` is the version to get.

I recommend you install pippi and work from a python virtualenv, which 
is built into the standard library in recent versions of python.

Create a virtualenv in a directory called `venv` with python 3.6:

    python3 -m venv venv

Activate the virtualenv:

    source venv/bin/activate

And install the libraries that pippi uses into the virtualenv:

    pip install -r requirements.txt

Pippi uses `numpy`, [pysndfile][psf], and Cython which all 
may create installation roadbumps depending on your platform.

If you're lucky, the above command ran without errors and you're 
good to go!

Otherwise, if there was a problem installing `numpy` please check out 
this page for more information about how to install it on your platform:

    https://www.scipy.org/scipylib/download.html

If there was a problem installing `pysndfile` you likely just need to install 
the library it wraps, `libsndfile`. 

On linux, you should be able to find `libsndfile` in your distro's package manager.

If you're on a mac, it looks like it can be installed with homebrew:

    http://brewformulas.org/Libsndfile

On windows, Erik de Castro Lopo provides an installer on the libsndfile page:

    http://www.mega-nerd.com/libsndfile/#Download

 
If you have trouble compiling the Cython parts of pippi, you may need to install 
the python development headers, and/or build tools for your system. More here:

    http://docs.cython.org/en/latest/src/quickstart/install.html

Please let me know if you run into problems!

More docs forthcoming as the deps for version 2 settle (especially 
as the interactive console comes together again.)

## Install from source

From inside your virtualenv, in the pippi source directory:

    python setup.py install

## Run the multi snare bounce example

    cd examples
    python multi_snare_bounce.py

Which will produce a WAV file named `multi_snare_bounce_example.wav` in the examples directory.

## To run tests

    make test

## Release Notes

### 2.0.0 - Alpha 3 (In Development)

Optimizations and improvements in this release:

- Much better performance for wavetable and granular synthesis
- Improvements and additions to the `rhythm` modules
    - Better handling of `rhythm.curve` which now takes a length param instead of an obscure combination of multipliers
    - `rhythm.curve` can now be provided a custom wavetable
    - Added MPC swing helper for onset lists (via `rhythm.swing`)
    - Added a euclidean rhythm generator `rhythm.eu`
    - Added a pattern generation helper `rhythm.pattern`
    - Added pattern-to-onset and string-to-pattern helpers
        - Patterns are the same as pippi 1 (I may even just port some code) and can be in a few forms:
            - String literals with ascii notation eg: 'xx x- x' which is the same as 'xx.x-.x'
            - Lists of 'truthy' and 'falsey' values eg: ['1', True, 0, False] which is the same as 'xx..'
- Misc bugfixes:
    - Fix `random` param for `wavetable.window` and `wavetable.window`
    - Fix bad params for `wavetable.window` and `wavetable.window` -- falls back to sine in both cases

### 2.0.0 - Alpha 2

This release includes a few missing pieces to core functionality including:

- A crude squarewave wavetype for the wavetable osc!
- Custom wavetables for the wavetable osc and window/wavetable generators! 
  See the `simple_custom_wavetable.py` example for use with the wavetable osc.
- A simple non-interpolating `speed` method on `SoundBuffer` for pitch shifting sounds
- A set of rhythm helpers in the `rhythm` module useful for constructing onset / timing lists.
    - `rhythm.curve` which lets you map any of the window types to a list of onsets -- check out 
    the `simple_snare_bounce.py` example in the examples directory.
- Some more example scripts including:
    - `simple_snare_bounce.py` Demoing the `rhythm.curve` helper
    - `multi_snare_bounce.py` A more interesting variation on the snare bounce example
    - `simple_custom_wavetable.py` Showing a user-defined wavetable used with `Osc`
    - `synth_chords.py` Using the `tune` module with `Osc` to create a simple chord progression

### 2.0.0 - Alpha 1

This is the initial alpha release of pippi 2 -- which is very barebones at the moment, 
but already pretty functional!

Beware: the behavior of core functionality and features will probably change throughout the 
alpha releases of pippi 2. I'll try to document it here in the release notes.

This release provides:

- SoundBuffer abstraction for reading/writing soundfiles and doing basic operations on sounds.
- Osc abstraction for simple wavetable synthesis.
- Initial set of built-in wavetables for windowing (sine, triangle, saw, inverse saw) 
  and synthesis (sine, cosine, triangle, saw inverse saw)
- Set of panning algorithms and other built-in sound operations like addition, subtraction, 
  multiplication, mixing (and operater-overloaded mixing via `sound &= sound`), dubbing, 
  concatenation.
- A small set of helpers and shortcuts via the `dsp` module for loading, mixing, and concatenating (via `dsp.join`) sounds.
- Basic granular synthesis and wavetable synthesis examples.

Some basic things from pippi 1 that are still missing (and coming very soon!):

- Pitch shifting sounds
- Square and impulse wavetables for windowing and synthesis
- Using arbitrary wavetables for windowing and synthesis

Bigger things from pippi 1 that are still missing:

- Pulsar synthesis
- The interactive / live mode and console which includes stuff like
    - MIDI support
    - Voice handling
    - Live code reloading

I'm excited to revisit the interactive console especially, and I'm looking into some cool 
stuff like using pippi instruments as generators to feed into sound streams, which should 
hopefully overcome some awkwardness of treating pippi 1 instruments like streams.


[psf]: https://forge.ircam.fr/p/pysndfile/
[lsf]: http://www.mega-nerd.com/libsndfile/
