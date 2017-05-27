# Pippi: Computer music with python

v2.0.0 - Alpha 2

## Install dependencies

Pippi requires python 3.5+

Pippi uses `numpy` and [pysndfile][psf]. `pysndfile` wraps [libsndfile][lsf]. (Both 
`pysndfile` and `libsndfile` need to be installed.) There are many ways to install numpy, and 
it may be as easy as `pip install numpy` on your platform. 
 
More docs forthcoming as the deps for version 2 settle (especially 
as the interactive console comes together again.)

In the meantime, please do open a bug on github if you try this 
out and have trouble installing it!

    pip install -r requirements.txt

## Install from source

    python setup.py install

## Run the multi snare bounce example

    cd examples
    python multi_snare_bounce.py

Which will produce a WAV file named `multi_snare_bounce_example.wav` in the examples directory.

## To run tests

    make test

## Release Notes

### 2.0.0 - Alpha 2

This release includes a few missing pieces to core functionality including:

- A crude squarewave wavetype for the wavetable osc!
- Custom wavetables for the wavetable osc and window/wavetable generators! 
  See the `simple_custom_wavetable.py` example for use with the wavetable osc.
- A simple non-interpolating `speed` method on `SoundBuffer` for pitch shifting sounds
- A set of rhythm helpers in the `rhythm` module useful for constructing onset / timing lists.
    - `rhythm.curve` which lets you map any of the window types to a list of onsets -- check out 
    the `simple_snare_bounce.py` example in the examples directory.

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
