from pippi import dsp, oscs, tune, fx, shapes

#LOOP = True

MIDI = ('MidiSport 2x2:MidiSport 2x2 MIDI 1 20:0', 0, 128)

def before(ctx):
    # The before callback is fired before 
    # each render -- but only once for each 
    # group of submix renders when yeilding 
    # over a polyphonic generator.
    ctx.log('before render')

def play(ctx):
    length = dsp.rand(0.3, 4)

    # ctx.p contains parameters passed with 
    # the triggering play command.
    # Play commands initiated by the MIDI relay 
    # will include the MIDI note as a parameter.
    # Parameters are provided as strings.
    note = float(ctx.p.note or dsp.randint(60, 70))
    #amp = 0.5 * (float(ctx.p.velocity or 0)/127)
    amp = 0.2

    # ctx.log will write a string to the system log
    ctx.log('note %s' % note)

    ctx.log('cache.foo %s' % ctx.cache.get('foo', None))

    # Convert the MIDI note to a frequency
    freq = tune.mtf(note)

    # This sets up a polyphonic generator 
    # by looping over a yield. Each buffer in 
    # the generator is rendered and then mixed 
    # together at the output.
    for _ in range(dsp.randint(1, 2)):
        # Set up the osc with a randomly detuned freq
        osc = oscs.SineOsc(freq=freq * dsp.rand(0.99, 1.01), amp=amp)

        # Render a buffer
        out = osc.play(length)

        # Add an envelope
        out = out.env('pluckout').taper(dsp.MS * 5)

        # Pan to a random position
        out = out.pan(dsp.rand())

        # Sometimes add foldback distortion
        if dsp.rand() > 0.5:
            out = fx.fold(out, dsp.rand(1,2))

        # Sometimes warble the pitch
        if dsp.rand() > 0.75:
            out = out.vspeed(shapes.win('sine', 0.5, 1.5))

        # Off to the mixer for playback
        yield out

def done(ctx):
    # The done callback is fired after each 
    # render completes.
    ctx.log('done rendering')

    # ctx.cache persists between renders.
    # Killing the renderer process will 
    # clear the cache. It is stored as a 
    # property on the renderer instance, 
    # not in the redis session.
    ctx.cache['foo'] = 'bar'
