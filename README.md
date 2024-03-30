Source code: [https://git.sr.ht/~hecanjog/pippi](https://git.sr.ht/~hecanjog/pippi)

Documentation: [https://pippi.world](https://pippi.world)

## Thanks

[Astrid Lindgren](https://en.wikipedia.org/wiki/Astrid_Lindgren) who wrote inspiring stories about Pippi Longstocking, this library's namesake.

[Will Mitchell](https://github.com/liquidcitymotors/) who contributed a wonderful zener diode softclip simulation, a state variable filter implementation available in the `fx` module, amazing work on bandlimiting in oscs and general moral support.

[Paul Batchelor](https://github.com/PaulBatchelor/Soundpipe) who created Soundpipe and sndkit, which pippi borrows greedily from for lots of super useful and fun DSP stuff.

[Project Nayuki](https://www.nayuki.io/page/free-small-fft-in-multiple-languages) who created a compact and understandable FFT used in `SoundBuffer.convolve()` among other places.

[Bernhard Schelling](https://zillalib.github.io/) for the TinySoundFont library used in the `soundfont` module.

[James McCartney](https://www.musicdsp.org/en/latest/Other/93-hermite-interpollation.html) who wrote the implementation of hermite interpolation used in the `Wavetable` module and elsewhere -- also, you know, supercollider of course! which lots of bits of pippi are inspired by or directly ported from -- see the libpippi sources for more info!

[Jatin Chowdhury](https://ccrma.stanford.edu/~jatin/ComplexNonlinearities/Wavefolder.html) who made the lovely saturating feedback wavefolder algorithm used in `fx.fold`.

[Nando Florestan](http://dev.nando.audio/) who made the small public domain GM soundfont used in the test suite.

[@noisesmith@sonomu.club](https://sonomu.club/@noisesmith) who introduced me to the modulation param on tukey windows...!
