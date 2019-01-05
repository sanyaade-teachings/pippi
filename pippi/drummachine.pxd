from pippi.soundbuffer cimport SoundBuffer

cdef class DrumMachine:
    cdef double bpm
    cdef dict drums

    cpdef void add(DrumMachine self, 
            str name, 
            object pattern=*, 
            SoundBuffer sound=*, 
            object callback=*, 
            double swing=*, 
            object lfo=*,
            double lfo_tempo=*, 
            double lfo_depth=*, 
            double delay=*
        )

    cpdef void update(DrumMachine self, str name, str param, object value)
    cpdef SoundBuffer play(DrumMachine self, double length)

