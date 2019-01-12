from pippi.soundbuffer cimport SoundBuffer
from pippi cimport rand
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
            list sounds=None, 
            object callback=None, 
            double swing=0, 
            double div=1, 
            object lfo=None,
            double delay=0
        ):
        self.drums[name] = dict(
            name=name, 
            pattern=pattern, 
            sounds=sounds, 
            callback=callback, 
            div=div, 
            swing=swing, 
            lfo=lfo, 
            delay=delay
        )

    cpdef void update(DrumMachine self, str name, str param, object value):
        self.drums[name] = self.drums.get(name, {}).update({param:value})

    cpdef SoundBuffer play(DrumMachine self, double length):
        cdef SoundBuffer out = SoundBuffer(length=length)
        cdef dict drum
        cdef list onsets
        cdef double onset
        cdef SoundBuffer clang
        cdef int count

        for k, drum in self.drums.items():
            onsets = rhythm.pattern(
                        pattern=drum['pattern'], 
                        bpm=self.bpm, 
                        div=drum['div'], 
                        length=length, 
                        swing=drum.get('swing', None), 
                        lfo=drum.get('lfo', None), 
                        delay=drum.get('delay', False),
                    )

            count = 0
            for onset in onsets:
                clang = SoundBuffer(filename=str(rand.choice(drum['sounds'])))
                if drum.get('callback', None) is not None:
                    clang = drum['callback'](clang, onset, count)
                out.dub(clang, onset)
                count += 1

        return out
