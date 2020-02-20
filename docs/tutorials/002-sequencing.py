
from pippi import dsp

out = dsp.buffer(length=90) # A 90 second long SoundBuffer

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

elapsed = 0


lfo = dsp.win('sinc', 0.1, 1) # Hat lengths between 100ms and 1s over a sinc window
lfo.graph('docs/tutorials/figures/002-sinc-win.png', label='sinc window')

while elapsed < out.dur:
    pos = elapsed / out.dur # position in the buffer between 0 and 1
    hatlength = lfo.interp(pos) # Sample the current interpolated position in the curve to get the hat length
    hat = makehat(hatlength)
    out.dub(hat, elapsed) # Finally, we dub the hat into the output buffer at the current time
    elapsed += 0.5 # and move our position forward again a half second so we can do it all again!

out.write('docs/tutorials/renders/002-hats-on-ice.wav')
