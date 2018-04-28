# Pippi: Computer music with python

v2.0.0 - Alpha 9

## Installation and setup

Pippi requires python 3 which can be found here:

    https://www.python.org/downloads/

I'm just testing against the latest stable version (currently 3.6.2) at the moment.

I recommend you install pippi and work from a python virtualenv, which 
is built into the standard library in recent versions of python.

Create a virtualenv in a directory called `venv` with python 3.6+:

    python3 -m venv venv

Activate the virtualenv:

    source venv/bin/activate

### Install from pip

To use the most recent release from pip, just:

    pip install pippi

...from inside your virtualenv.

### Install from source

Or to install the most recent development version, clone this repo 
and install with the `-e` / 'editable' option. (Also note the `.` pointing to the current working directory.)

    pip install -e .

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

## Run the multi snare bounce example

    cd examples
    python multi_snare_bounce.py

Which will produce a WAV file named `multi_snare_bounce_example.wav` in the examples directory.

## Now what?

There are more examples, give em a whirl, and try your own. 
(And let me know when things break please!)

## To run tests

    make test

## Release Notes

### 2.0.0 - Alpha 9

Final feature releases / big api changes before going to beta.

- Breaking changes:
    - Durations for most APIs are now given in seconds (floats) rather than 
      integer frames. `len(SoundBuffer)` still returns a length in frames per
      python convention, and slicing into a `SoundBuffer` is also still done by frame
      (and channel) but there is a new `SoundBuffer.dur` property to get duration 
      in seconds as well.
    - Wavetables are no longer specified with string names, instead built-in 
      flags which are available on both the `wavetable` and `dsp` modules are 
      used. Eg to apply a sinewave envelop: `sound.env(dsp.SINE)` instead of `sound.env('sine')`. 
      The wavetypes available are `SINE`, `COS`, `TRI`, `SAW` (which is also aliased to 
      `PHASOR`), `RSAW` (reverse sawtooth), `HANN`, `HAMM`, `BLACK` or `BLACKMAN`, 
      `BART` or `BARTLETT`, `KAISER`, `SQUARE`, and the `RND` flag to select one at random.
- `Osc` changes:
    - Added 2d wavetable synthesis (similar to max/msp `2d.wave~`) to `Osc` plus example script
    - To create a 2d `Osc`, use the `stack` keyword arg on initialization: `Osc(stack=[dsp.RND, [0,1], dsp.SINE], lfo=dsp.SINE)`
    - `Osc` wavetables may be:
        - an int flag for standard wavetables (dsp.SINE, dsp.TRI, etc)
        - a python list of floats
        - a wavetable instance
        - a soundbuffer
    - 2d wavetable stacks are a python list of any combination of the above.
    - The same types are acceptable for:
        - `wavetable` (the basic waveform)
        - `window` (an optional window to apply to the waveform wavetable - useful for eg pulsar synthesis)
        - `mod` (the frequency modulation wavetable)
        - and `lfo` (the 2d modulation wavetable)
- `SoundBuffer` changes:
    - Added `remix` for remixing a soundbuffer from N channels to N channels.
    - Panning algorithms operate on arbitrary numbers of channels (but use same algorithms applied to odd & even numbered channels instead of left & right)
    - Return a reversed copy of a soundbuffer with `sound.reversed()` or reverse in place with `sound.reverse()`
    - New ADSR envelopes with `sound.adsr(a=1, d=1, s=0.5, r=1)`
    - Generate a `GrainCloud` from a `SoundBuffer` with `sound.cloud()`
    - Clip samples to min/max with `sound.clip(minval=-1, maxval=1)`
    - Taper ends of sounds (linear fade-in, fade-out) with `sound.taper(length)`
- ADSR wavetable generator with `wavetables.adsr(a=100, d=100, s=0.5, r=100, 1024)`
- New `Wavetable` type for `SoundBuffer`-like operator-overloaded wavetable manipulation & composition
- New `GrainCloud` wavetable-driven granulator. See the `examples/swarmy_graincloud.py` example for more.
- `GrainCloud`-driven pitch shift without time change (`sound.transpose(speed)`) 
   and time stretch without pitch shift (`sound.stretch(length)`) methods for `SoundBuffer`.
- `dsp.cloud(SoundBuffer, *args, **kwargs)` shortcut for `GrainCloud` creation.
- Read wavetables from 1 channel sound files with `wavetables.fromfile`
- Added a simple helper for async rendering with `multiprocessing.Pool`
- `SoundBuffer`s can now be pickled (enables passing them between processes)
- `SoundBuffer` can be initialized (and spread across channels) from a normal python list


### 2.0.0 - Alpha 6-8

This was meant to be a feature-only release, to add the final round of features 
before going into beta / bugfix mode. Instead I switched from using numpy arrays directly 
to a first pass of a more general typed memoryview approach, and moved some more things into 
cython.

Also, hoo boy was `pip install pippi` ever *broken*.
It should be working now.

### 2.0.0 - Alpha 5

Bugfix release. Fixed an idiotic regression in `SoundBuffer`.

### 2.0.0 - Alpha 4

New in alpha 4:

- More speed improvements!
- Linear interpolation option for pitch shifting
- Pulsar synthesis with `Osc`
- Support for importing Scala `.scl` tuning files (Mapping file support coming later...)
- Waveform visualization with `graph.waveform`
- `SoundBuffer.fill` returns a copy of the sound instead of altering it in place
- `Sampler` abstraction for `Osc`-like treatment of samples and banks of samples

### 2.0.0 - Alpha 3

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
