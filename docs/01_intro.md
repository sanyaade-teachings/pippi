# Unfinished!

---

## How does pippi work?

Pippi takes advantage of a few features of CPython:

- Doing string manipulations with python's internal C methods (like `string.join()`) is fast.
- The python standard library includes the `audioop` module (DSP code ripped straight out of the excellent SoX command line utility) which accepts audio as a binary string literal and returns the same. It is also fast.
- There's a handy `wave` module in the standard library, which makes importing and exporting PCM wave data simple.

### Data format

Internally, all sound in pippi is stored and manipulated as binary **string literals**. 

*What?*

Python's approach to working with binary data is to use its string data type as a wrapper. To get a 
feel for this, lets first look at the structure of the type of binary audio data we're trying to represent.

Signed 16 bit PCM audio has these characteristics:

Each frame of audio represents an instantanious value corresponding to a position the speaker cones will take 
when sent to our computer's digital-to-analog converter. *PCM* stands for Pulse Code Modulation - the modulation 
part refers to the filtering needed to correct for aliasing distortion that would occur if an analog speaker simply 
jumped from value to value over time, rather than somehow smoothly interpolating between these values.

It is conventional to use signed 16 bit integers to represent an instantanious speaker cone position - this 
is also the format CD audio takes.

A signed integer means that instead of having a range between zero and some positive number, it has a range 
between some negative number and some positive number.

This is great for representing audio - at zero the speaker cone is at rest, at max it is pushed as far out as 
it can go, and at min it is pulled as far in as it can go.

The number of bits in the integer dictate the size of the number it is possible to represent.

A 16 bit integer can represent `2^16` discrete values - or `65,536` total values.

That means a signed integer will use about half of those possible values as positives, and half as negatives.

Half of `2^16` is `2^15`, or `32,768`. Because we need to account for a zero value, the range of our signed 16 bit integer 
is actually `-2^15` to `2^15 - 1`. Or `-32,768` to `32,767`.

The potential size of the integer - or the number of discrete values it can represent - corresponds to the 
possible dynamic range that can be represented in the audio recording. More values mean more subtle differences 
between loud sounds and soft sounds, and everything in between. [Bhob Rainey has a wonderful writeup on why this 
is something to pay attention to.](http://bhobrainey.wordpress.com/2010/08/04/selected-occasions-of-handsome-deceit/) (Also 
his music rules, so be sure to check it out.)

All that said, it's fairly accepted that 16 bit audio can represent differences in loudness that comes close to the 
limit our brains can distinguish. Supporting higher bit rates in pippi is on the list of to dos, but only because that 
extra dynamic resolution becomes useful when you're transforming very quiet sounds, or sounds with a very limited dynamic range.

So, we could just work with lists of python integers, but doing operations in pure python can get pretty slow - 
especially when a system will quickly grow to working with minutes and hours of audio. By relying on the fast C 
backend for string manipulation and basic DSP, performance is actually pretty good.

Instead we represent each integer as a python string, and when doing synthesis, use the `struct` module to 
pack our integers into string literals.

To turn the python integer `32,767` into a string literal, we can give `struct.pack` a format argument and 
it will convert the number into the correct binary string literal.

    >>> import struct
    >>> struct.pack('<h', 32767)
    '\xff\x7f'

While it looks like we just got back an eight character string, this is actually a two character string, 
where the leading `\x` is the way python indicates that the next two characters should be read as a hex value.

So if we have `44,100` frames of a single channel of 16 bit audio, internally we'd have a string whose length 
is actually twice that - `88,200` characters. With two channels, our string will have `176,400` characters.

One convenience pippi provides is a way to prevent you from accidentally splitting a sound in the middle 
of a two-character word, which will instantly turn your audio into a brand new Merzbow track.

If you have a 10 frame 2 channel sound (in the below example, silence) and want to grab the last 5 frames, 
you could do:

    >>> import struct
    >>> sound = struct.pack('<h', 0) * 2 * 10
    >>> len(sound)
    40
    >>> sound = sound[:5 * 2 * 2]
    >>> len(sound)
    20

Five frames of stereo 16 bit audio represented as a string will have a length of 20 characters.

You may see how this could get annoying, fast. And an off-by-one error will produce Merzbow forthwith.

With pippi, to do the same, we use the cut method:

    >>> from pippi import dsp
    >>> dsp.flen(sound)
    10
    >>> sound = dsp.cut(sound, 5, 5)
    >>> dsp.flen(sound)
    5 

Using the same silence 10 frames from the earlier example, we can check the actual length with `dsp.flen()`. 
(Which is short for 'frame length' or 'length in frames')

To do the cut, `dsp.cut()` accepts three params: first, the string literal to cut from, next the offset in frames 
where the cut should start, and third the length of the cut in frames. 

### Summary

Part of what pippi provides is a wrapper to working with python string literals. This is actually a very handy thing.
That's just a small part of the library though. Next we'll talk about doing basic synthesis with pippi, and using some of 
its waveshape generators for both audio and control data.
