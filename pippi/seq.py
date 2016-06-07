import mido
import dsp, tune
from params import ParamManager

class Pattern(object):
    def __init__(self, length, pat=None, deg=None, div=1, id=0, pos=0, ns=None):
        self.id     = id
        self.ns     = ns
        self.params = ParamManager(self.ns)

        deg = [ int(d) for d in deg.split(',') ]
        setattr(self.ns, 'pattern-deg-%s' % self.id, deg)
        setattr(self.ns, 'pattern-pos-%s' % self.id, pos)
        setattr(self.ns, 'pattern-pat-%s' % self.id, pat)
        setattr(self.ns, 'pattern-len-%s' % self.id, length)
        setattr(self.ns, 'pattern-div-%s' % self.id, div)
        setattr(self.ns, 'pattern-oct-%s' % self.id, self.params.get('octave'))

        self.fill(length)

    def __str__(self):
        return 'p%s: %s/%s o:%s %s %s' % (
                self.id, 
                getattr(self.ns, 'pattern-len-%s' % self.id), 
                getattr(self.ns, 'pattern-div-%s' % self.id), 
                getattr(self.ns, 'pattern-oct-%s' % self.id), 
                getattr(self.ns, 'pattern-pat-%s' % self.id), 
                getattr(self.ns, 'pattern-deg-%s' % self.id) 
            )

    def all(self):
        return getattr(self.ns, 'pattern-seq-%s' % self.id, [])

    def reset(self):
        setattr(self.ns, 'pattern-pos-%s' % self.id, 0)

    def next(self):
        pos = getattr(self.ns, 'pattern-pos-%s' % self.id)
        seq = getattr(self.ns, 'pattern-seq-%s' % self.id)
        deg = getattr(self.ns, 'pattern-deg-%s' % self.id)
        octave = getattr(self.ns, 'pattern-oct-%s' % self.id)

        value = seq[pos % len(seq)]

        note = deg[pos % len(deg)]
        note = tune.fromdegrees([note], octave=octave, root=self.params.get('key', 'c'))[0]
        
        pos += 1

        setattr(self.ns, 'pattern-pos-%s' % self.id, pos)

        length = dsp.mstf(40, 120)
        return (note, value, length)

    def fill(self, length, pat=None):
        if pat is None:
            pat = getattr(self.ns, 'pattern-pat-%s' % self.id, [])

        if 'eu' in pat:
            euparams = pat[2:]
            if ':' in euparams:
                euparams = euparams.split(':')
                numbeats = int(euparams[0])
                offset = int(euparams[1])
            else:
                numbeats = int(euparams)
                offset = 0
            
            seq = dsp.eu(length, numbeats, offset)

        else:
            seq = self.unpack(length, pat)

        setattr(self.ns, 'pattern-seq-%s' % self.id, seq)

    def unpack(self, length, pat):
        rests = ('.', ' ', ',', '-', 0)
        pat = [ pat[i % len(pat)] for i in range(length) ]
        for i, p in enumerate(pat):
            if p in rests:
                pat[i] = 0
            else:
                pat[i] = 1

        return pat


class Lfo(object):
    """

    # Starts a new voice playing the LFO directly
    lfo N <lfo params>

    lfo # <lfo params>

    """
    def __init__(self, length, wt=None, pos=0):
        self.fill(length, wt)

    def fill(self, length, wt):
        self.seq = dsp.wavetable(wt, length)

