#cython: language_level=3

from pippi.wavetables cimport Wavetable
from pippi.soundbuffer cimport SoundBuffer

cpdef list topositions(object p, double beat, double length, Wavetable lfo=*)
cpdef list frompattern(object pat, double bpm=*, double length=*, double swing=*, double div=*, object lfo=*, double delay=*)

cdef class Seq:
    cdef double bpm
    cdef public dict drums

    cpdef void add(Seq self, 
            str name, 
            object pattern=*, 
            object callback=*, 
            object sounds=*, 
            object barcallback=*, 
            double swing=*, 
            double div=*, 
            object lfo=*,
            double delay=*
        )

    cpdef SoundBuffer play(Seq self, double length)

