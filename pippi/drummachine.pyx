from pippi.soundbuffer cimport SoundBuffer
from pippi import rhythm

cdef class DrumMachine:
    def __init__(DrumMachine self, object bpm=None, dict drums=None):
        if bpm is None:
            bpm = 120.0
        self.bpm = bpm

        if drums is None:
            drums = {}

        self.drums = drums

    cpdef void add(DrumMachine self, 
            str name, 
            object pattern=None, 
            SoundBuffer sound=None, 
            object callback=None, 
            double swing=0, 
            object lfo=None,
            double lfo_tempo=1, 
            double lfo_depth=0, 
            double delay=0
        ):
        self.drums[name] = self.drums.get(name, {}).update(dict(
            name=name, 
            pattern=pattern, 
            sound=sound, 
            callback=callback, 
            swing=swing, 
            lfo=lfo, 
            lfo_tempo=lfo_tempo, 
            lfo_depth=lfo_depth, 
            delay=delay))

    cpdef void update(DrumMachine self, str name, str param, object value):
        self.drums[name] = self.drums.get(name, {}).update({param:value})

    cpdef SoundBuffer play(DrumMachine self, double length):
        cdef SoundBuffer out = SoundBuffer(length=length)
        cdef dict drum
        cdef list onsets
        cdef double onset
        cdef SoundBuffer clang
        cdef int count

        for drum in self.drums:
            onsets = rhythm.pattern(
                        pattern=drum['pattern'], 
                        bpm=self.bpm, 
                        length=length, 
                        swing=drum.get('swing', None), 
                        lfo=drum.get('lfo', None), 
                        lfo_tempo=drum.get('lfo_tempo', 1), 
                        lfo_depth=drum.get('lfo_depth', 0), 
                        delay=drum.get('delay', False),
                    )

            count = 0
            for onset in onsets:
                clang = drum.get('sound', None)
                if drum.get('callback', None) is not None:
                    clang = drum['callback'](clang, onset, count)
                out.dub(clang, onset)
                count += 1

        return out
