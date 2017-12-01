cdef class SoundBuffer:
    cdef public int samplerate
    cdef public int channels
    cdef public double[:,:] frames

    cdef void _fill(self, double[:,:] frames)
    cdef void _dub(self, SoundBuffer sound, int framepos=*)
    cdef double[:,:] _speed(self, double speed)
