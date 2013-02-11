## Sloshy sines

    from pippi import dsp
    from pippi import tune

    print 'Starting at timestamp:', dsp.timer('start')

    def swell(freq, numpartials, length):
        """ Generate a stack of sine partials
            on a given root freq.
            """

        # Generate a list of sines
        out = [ dsp.tone(length, freq * partial, amp=0.1) for partial in range(1, numpartials) ]

        # Apply amplitude envelopes to each partial
        out = [ dsp.env(partial, dsp.randchoose(['hann', 'tri', 'sine', 'line', 'phasor'])) for partial in out ]

        # Pan each partial to a random position
        out = [ dsp.pan(partial, dsp.rand(0, 1)) for partial in out ]

        # Modulate the speed of each partial by a small amount
        out = [ dsp.drift(partial, dsp.rand(0.0, 0.03)) for partial in out ]

        # Mix the partials into a single sound
        return dsp.mix(out)

    freqs = tune.fromdegrees([1,3,4,5,6,9], octave=2, root='a')

    layers = []

    for i in range(5):
        layers += [ ''.join([ swell(freq, dsp.randint(4, 10), dsp.stf(dsp.rand(2, 8))) for freq in dsp.randshuffle(freqs) ]) ]

    out = dsp.mix(layers)

    print 'Render took', dsp.timer('stop'), 'seconds'
    print 'Listen to this:', dsp.write(out, 'sloshy')



