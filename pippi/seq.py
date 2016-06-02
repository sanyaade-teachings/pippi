import mido
import dsp, tune
from params import ParamManager

class Pattern(object):
    def __init__(self, length, pat=None, notes=None, div=1, id=0, pos=0, ns=None):
        self.pos    = pos
        self.pat    = pat
        self.notes  = notes
        self.degrees = [ int(d) for d in notes.split(',') ]
        self.length = length
        self.id     = id
        self.div    = div
        self.ns     = ns
        self.params = ParamManager(self.ns)
        self.octave = self.params.get('octave') 

        self.fill(length)

    def __str__(self):
        return 'p%s: %s/%s o:%s %s %s' % (self.id, self.length, self.div, self.octave, self.pat, self.notes)

    def all(self):
        return self.seq

    def reset(self):
        self.pos = 0

    def next(self):
        value = self.seq[self.pos % len(self.seq)]

        note = self.degrees[self.pos % len(self.degrees)]
        note = tune.fromdegrees([note], octave=self.octave, root=self.params.get('key', 'c'))[0]
        
        self.pos += 1
        length = dsp.mstf(40, 120)
        return (note, value, length)


    def fill(self, length):
        if 'eu' in self.pat:
            euparams = self.pat[2:]
            if ':' in euparams:
                euparams = euparams.split(':')
                numbeats = int(euparams[0])
                offset = int(euparams[1])
            else:
                numbeats = int(euparams)
                offset = 0
            
            self.seq = dsp.eu(length, numbeats, offset)

        else:
            self.seq = self.unpack(length, self.pat)

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
