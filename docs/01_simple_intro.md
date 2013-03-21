Pippi is a system designed for creating computer music. It tries to stay out of your way 
whenever it can and let you write code however you like.

Lets start with this piano recording and try some things with pippi:

    [ embed piano recording ]

One of my favorite things is layering pulses of different rates. Computers are really 
good at this and let us do some crazy & fun stuff.

First, lets load the piano recording with pippi:

    # Import the Sound class from pippi
    from pippi import Sound

    # Load our piano recording
    piano = Sound('piano.wav')

Lets try making 20 layers of piano, each with a different pulse and each 
panned to a different stereo position.

If we want 20 different layers, we can slice the recording into 20 different 
short parts first. Lets make the parts overlap a little though.

    # Grab a handy unit - the length of one millisecond
    from pippi import milliseconds as MS 

    # Slice our piano recording into a list of 20 slices, between 20 and 300 milliseconds
    # long. The start of the next slice should overlap with the end of the last slice by 
    # 50%. Check out the graphic below for an illustration.
    parts = piano.slice(minlength=20*ms, maxlength=300*ms, overlap=0.5, slices=20)

Now all we need to do is copy each slice enough times to fill one layer, then pan the 
layers and mix everything together!

    # Grab another handy unit - the length of one second
    from pippi import seconds as S

    # Make an empty list to hold our piano layers
    layers = []

    # Loop through the slices and for each one...
    for slice in slices:
        # ...copy it enough to fill 10 seconds of time...
        slice = slice.fill(10 * S)

        # ...pan it to a random position...
        slice = slice.pan('random')

        # ...and append the slice to our list of layers.
        layers.append(slice)

The last step is just to mix our layers together and save the new sound!

    # Grab some handy tools we'll need from pippi's dsp module
    from pippi import dsp

    # Mix our layers together
    out = dsp.mix(layers)

    # Save the sound as 'piano_slices.wav'
    dsp.write(out, 'piano_slices.wav')
