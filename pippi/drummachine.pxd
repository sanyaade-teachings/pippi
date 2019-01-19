from pippi.soundbuffer cimport SoundBuffer

cdef class DrumMachine:
    cdef double bpm
    cdef public dict drums

    cpdef void add(DrumMachine self, 
            str name, 
            object pattern=*, 
            object sounds=*, 
            object callback=*, 
            object barcallback=*, 
            double swing=*, 
            double div=*, 
            object lfo=*,
            double delay=*
        )

    cpdef SoundBuffer play(DrumMachine self, double length)

