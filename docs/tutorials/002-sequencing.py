
from pippi import dsp

lowhz = dsp.win('hannin', 9000, 11000)

# Graph it
lowhz.graph('docs/tutorials/figures/002-lowhz-hann-curve.png')

highhz = dsp.win('hannin', 12000, 14000)

from pippi import noise 

hat = noise.bln('sine', dsp.MS*80, lowhz, highhz)

pluckout = dsp.win('pluckout')
pluckout.graph('docs/tutorials/figures/002-pluckout.png')

hat = hat.env(pluckout) * 0.5 # Finally multiply by half to reduce the amplitude of the signal
hat.write('docs/tutorials/renders/002-plucked-hat.ogg')


def makehat(length=dsp.MS*80):
    lowhz = dsp.win('rnd', 9000, 11000)
    highhz = dsp.win('rnd', 12000, 14000)
    return noise.bln('sine', length, lowhz, highhz).env(pluckout) * 0.5


lfo = dsp.win('sinc', 0.1, 1) # Hat lengths between 100ms and 1s over a sinc window
lfo.graph('docs/tutorials/figures/002-sinc-win.png', label='sinc window')

out = dsp.buffer(length=30)

elapsed = 0
while elapsed < 30: 
    pos = elapsed / 30 # position in the buffer between 0 and 1
    hatlength = lfo.interp(pos) # Sample the current interpolated position in the curve to get the hat length
    hat = makehat(hatlength)
    out.dub(hat, elapsed) # Finally, we dub the hat into the output buffer at the current time
    elapsed += 0.5 # and move our position forward again a half second so we can do it all again!

out.write('docs/tutorials/renders/002-hats-on-ice.ogg')

from pippi import fx # The hats sound nice with a butterworth lowpass

length_lfo = dsp.win('sinc', 0.1, 1) 
time_lfo = dsp.win('hann', 0.001, 0.2) # Time increments between 1ms and 200ms over a sinc curve
time_lfo.graph('docs/tutorials/figures/002-time-lfo.png', label='time lfo')

out = dsp.buffer(length=30) # Otherwise we'll keep adding to our last buffer

elapsed = 0 # reset the clock
while elapsed < 30:
    # Our relative position in the buffer from 0 to 1
    pos = elapsed / 30

    # The length of the hat sound, sampled from the value of a sinc curve
    # at the current position in the output buffer
    hatlength = length_lfo.interp(pos) 

    # Call out hi hat function which returns a SoundBuffer
    hat = makehat(hatlength)

    # Dub the hi hat sound into our output buffer at the current position in seconds
    out.dub(hat, elapsed) 

    # Sample a length from the time_lfo at this position to determine how far ahead to 
    # move before the loop comes back around to dub another hat.
    beat = time_lfo.interp(pos)
    elapsed += beat

# Add a butterworth lowpass with a 3k cutoff
out = fx.lpf(out, 3000)
out.write('docs/tutorials/renders/002-hats-slipping-on-ice.ogg')
