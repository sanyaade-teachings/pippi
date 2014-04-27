Note: the pippi console currently depends on ALSA, and is therefore linux-only. [(The core dsp system is however fully cross platform.)](https://github.com/hecanjog/pippi)

Quick start
===========

Pippi only supports python 2.7.x at the moment, so verify that's what you're using

    $ python --version

### Install pippi-console for all users from source

    $ sudo python setup.py install

### Create a new pippi project

    $ pippi new acoolproject

### Start pippi

    $ cd acoolproject
    $ pippi

### Run the example generator script

    ^_- ex re

The `^_- ` is just the cheeky prompt for the pippi console. 

The first command `ex` is the generator script's 'shortname'. (Short for example of course!)

The second command `re` tells pippi to regenerate the buffer produced by the script after each play, so as the 
output of the script loops, you may modify it to change the behavior.

While the generator plays, open the file 'example.py' in the 'orc' directory with your favorite text editor and 
find the line that reads `freq = tune.ntf('a', octave=2)`. 

Try changing `'a'` to `'e'` or `'f#'`. Or maybe try changing `octave=2` to `octave=4` or `octave=10`. Or find 
`sine2pi` and try changing it to `tri`, `impulse`, `vary` or `hann`.

Back in the pippi console, type `i` to get a list of the currently playing voices

    ^_- i
    01 gen: EX dev: default bpm:120.0

Change the global bpm

    ^_- bpm 110

Stop the currently running voice - we give the `s` command a param `1` because that's the id of the voice 
we'd like to stop.

    ^_- s 1

Start a group of 10 voices with the example generator

    ^_- group 10 -- ex re

Pippi just sends each voice stream to alsa for summing, so we get some interesting distortion effects 
because every sound is playing very loudly.

Have each voice choose a random volume on each iteration. Find the appropriate line and change it to read:

    volume = P(voice_id, 'volume', default=dsp.rand(0, 20)) / 100.0

Above we're trying to read a parameter passed in at the pippi console to set the volume. Since we didn't 
give the generator any volume param when we started our voices, it uses the default value passed as the 
third argument to `P` which in this case is a random number between 0 and 100, updated on each iteration.

Try setting the 5th voice's volume to 50. We can use the `u` or update command to change a running voice's 
parameters as it plays.

    ^_- u 5 v:50

Now lets make the rhythm a bit more interesting. Find the line where a value is assigned to `beat` and 
update it to read

    beat = dsp.bpm2frames(bpm) / dsp.randint(1, 4)

Vary the chosen octave

    freq = tune.ntf('a', octave=dsp.randint(1, 5))

Randomly choose from a given set of pitches

    freq = tune.ntf(dsp.randchoose(['a', 'c#', 'f#'], octave=dsp.randint(1, 5)))

Apply an amplitute envelope to the beep. After line 34 (where we set `out` to our beep sound) and before line 
37 (where `out` is passed into `dsp.pad` and silence is appended to the end) try adding

    out = dsp.env(out, 'sine')

Each voice begins playing as soon as rendering is finished, and the last iteration has also finished playing. To 
force voice playback to quantize to the master bpm, we can give any voice the `qu` command.

To update every voice spawned from the example generator, adding the `qu` command, type

    ^_- uu ex qu

To stop every voice currently playing (but allow each iteration to play though before stopping)

    ^_- ss

Check out the documentation for pippi for more code examples you can use in generator scripts. 
I also have a small but growing collection of instruments and recipes for pippi in my [hcj.py](https://github.com/hecanjog/hcj.py) repo.

