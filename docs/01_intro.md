# Unfinished!

---

## How does pippi work?

Pippi takes advantage of a few features of CPython:

- Doing string manipulations with python's internal C methods (like string.join()) is fast.
- The python standard library includes the audioop module (DSP code ripped straight out of the excellent SoX command line utility) which expects audio input as a binary string and returns the same. It is also fast.
- There's a handy wave module in the standard library, which makes importing and exporting PCM wave data simple.

### Data format

Internally, all sound in pippi is stored and manipulated as binary **string literals**. 

*What?*

Python's approach to working with binary data is to use its string data type as a wrapper. To get a 
feel for this, lets first look at the structure of the type of binary audio data we're trying to represent.

*Signed 16 bit PCM audio* has these characteristics:

Each frame of audio represents an instantanious value corresponding to a position the speaker cone will take 
when sent to our computer's digital-to-analog converter. *PCM* stands for Pulse Code Modulation.

It is conventional to use signed 16 bit integers to represent an instantanious speaker cone position - this 
is also the format CD audio takes. 

A signed integer means that instead of having a range between 0 and some positive number, it has a range 
between some negative number and some positive number.

This is great for representing audio - at zero the speaker cone is at rest, at max it is pushed as far out as 
it can go, and at min it is pulled as far out as it can go.

The number of bits in the integer dictate the size of the number it is possible to represent.

A 16 bit integer can represent 2^16 discrete values - or 65,536 total values, including zero.

That means a signed integer will use half of those possible values as positives, and half as negatives.

Half of 2^16 is 2^15, or 32,768. Because we need to account for a zero value, the range of our signed 16 bit integer 
is actually -2^15 to 2^15 - 1. Or -32,768 to 32,767.

We could just work with lists of python integers internally, but doing operations on them is pretty slow - 
especially when a system will quickly grow to working with minutes and hours audio.

Instead, we represent each integer as a python string. When doing synthesis, the struct module is useful.

To turn the python integer 32,767 into a binary string, we can give struct.pack a special format argument and 
it will convert the number into a string.

    >>> import struct
    >>> struct.pack('<h', 32767)
    '\xff\x7f'

While it looks like we just got back an eight character string, this is actually a two character string, 
where the leading \x tells python to interpret the next two characters as a hex value.

So if we have 44,100 frames of a single channel of 16 bit audio, internally we'd have a string whose length 
is actually twice that - 88,200 characters. With two channels, our string will have 176,400 characters.

One convenience pippi tries to provide is to prevent you from accidentally splitting a sound in the middle 
of a two-character word, which will instantly turn your audio into a brand new Merzbow track.

To cut 100 ms of audio out of a sound, use the cut method:

    >>> from pippi import dsp
    >>> sound = dsp.read('mysound.wav')
    >>> dsp.flen(sound)
    44100
    >>> short_sound = dsp.cut(sound, 0, dsp.mstf(100))
    >>> dsp.flen(short_sound)
    4410
    >>> dsp.flen(sound)
    44100

First, we read a WAV file that is exactly 1 second long and store it as a string into the 'sound' variable.

Next, we can check the length in frames with the dsp.flen() method, which will always give you a length in 
frames, no matter how many channels of audio.

To do the cut, dsp.cut() accepts three params: first, the sound string to cut from, next the offset in frames 
where the cut should start, and third the length of the cut in frames. The function dsp.mstf() is an example of 
some of the unit conversation utilities pippi provides. It will convert milleseconds to frames at the current 
sampling rate. (Which is always 44100 - more on that later.)

Finally, we can see the original sound has not been altered. Python strings are not mutable, and all cut does 
is actually return a slice of the original sound - it may have been more helpfully called 'copy_segment' but that's 
a bit long to type. :-)


