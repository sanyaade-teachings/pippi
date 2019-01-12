from pippi.soundbuffer cimport SoundBuffer

cdef class DrumMachine:
    cdef double bpm
    cdef public dict drums

    cpdef void add(DrumMachine self, 
            str name, 
            object pattern=*, 
            list sounds=*, 
            object callback=*, 
            double swing=*, 
            double div=*, 
            object lfo=*,
            double delay=*
        )

    cpdef void update(DrumMachine self, str name, str param, object value)
    cpdef SoundBuffer play(DrumMachine self, double length)

