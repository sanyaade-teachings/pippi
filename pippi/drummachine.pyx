#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer
from pippi cimport rand
from pippi import rhythm

cdef class DrumMachine:
    def __init__(DrumMachine self, object bpm=None):
        if bpm is None:
            bpm = 120.0
        self.bpm = bpm
        self.drums = {}

    cpdef void add(DrumMachine self, 
            str name, 
            object pattern=None, 
            object sounds=None, 
            object callback=None, 
            object barcallback=None, 
            double swing=0, 
            double div=1, 
            object lfo=None,
            double delay=0
        ):
        self.drums[name] = dict(
            name=name, 
            pattern=pattern, 
            sounds=list(sounds), 
            callback=callback, 
            barcallback=barcallback, 
            div=div, 
            swing=swing, 
            lfo=lfo, 
            delay=delay
        )

    cpdef SoundBuffer play(DrumMachine self, double length):
        cdef SoundBuffer out = SoundBuffer(length=length)
        cdef SoundBuffer bar
        cdef dict drum
        cdef list onsets
        cdef double onset
        cdef SoundBuffer clang
        cdef int count

        for k, drum in self.drums.items():
            bar = SoundBuffer(length=length)
            onsets = rhythm.pattern(
                        drum['pattern'], 
                        bpm=self.bpm, 
                        div=drum['div'], 
                        length=length, 
                        swing=drum['swing'], 
                        lfo=drum['lfo'], 
                        delay=drum['delay'],
                    )

            count = 0
            for onset in onsets:
                clang = SoundBuffer(filename=str(rand.choice(drum['sounds'])))
                if drum.get('callback', None) is not None:
                    clang = drum['callback'](clang, onset, count)
                bar.dub(clang, onset)
                count += 1

            if drum.get('barcallback', None) is not None:
                bar = drum['barcallback'](bar)
            out.dub(bar)

        return out
