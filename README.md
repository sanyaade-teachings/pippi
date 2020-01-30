[Pippi: Computer music with python](banner.png)

## Pippi: Computer music with python

v2.0.0 - Beta 4 (In Development)

## What is this?

Pippi is a computer music system that aims to let you write python scores for offline rendering. 
`astrid` is the experimental kinda-realtime counterpart to `pippi` -- a DAW/live-coding environment 
for performance and interactive applications.

## Installation

Pippi requires python 3.6+ which can be found here:

    https://www.python.org/downloads/

The 3.5.x branch of python might work too, but is untested.

To use the most recent release from pip (currently `2.0.0 beta 3`) just:

    pip install pippi

*But!* Please see below about installing the latest version from source, there are a lot of new features in the most recent beta.

## Tutorials

There are annotated example scripts in the [tutorials](tutorials) directory which introduce some of pippi's functionality.

Beyond arriving at a good-enough stable API for the 2.x series of releases (and fixing bugs), my goal during the 
beta phase of development is to deal with the lack of documentation for this project.

## Install from source

> raspbian buster users: you must install the `libatlas-base-dev` package with `apt` to build the latest version of numpy.

To install pippi:

    make install

Which does a few things:

- Installs python deps, so *make sure you're inside a virtual environment* if you want to be!
- Sets up git submodules for external libs
- Builds and installs Soundpipe
- Builds and installs pippi & cython extensions

Please let me know if you run into problems!

## To run tests

    make test

In many cases, this will produce a soundfile in the `tests/renders` directory for the corresponding test. (Ear-driven regression testing...)
During the beta I like to keep failing tests in the main repo, so... most tests will be passing but if they *all* are passing, probably you are living in the future and are looking at the first stable release.

There are also shortcuts to run only certain groups of tests, like `test-wavesets` -- check out the `Makefile` for a list of them all.

## Hacking

While hacking on pippi itself, running `make build` will recompile the cython extensions.

If you need to build sources from a clean slate (sometimes updates to `pxd` files require this) then run `make clean build` instead.

## Thanks

[Project Nayuki](https://www.nayuki.io/page/free-small-fft-in-multiple-languages) for a compact FFT! (Used in `SoundBuffer.convolve()`)

[Paul Batchelor](https://github.com/PaulBatchelor/Soundpipe) for all the goodness in Soundpipe that has made its way into Pippi. (See the `fx` and `bar` modules.)

[Bernhard Schelling](https://zillalib.github.io/) for his TinySoundFont library used in the `soundfont` module.

[Nando Florestan](http://dev.nando.audio/) for his small public domain GM soundfont used in the test suite.

[Pixeldroid](https://github.com/pixeldroid/fonts) for their OFL licensed console font used for labeling graphs.


