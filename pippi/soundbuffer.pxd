cdef class SoundBuffer:
    cdef public int samplerate
    cdef public int channels
    cdef public double[:,:] frames

    cdef SoundBuffer _adsr(self, double attack, double decay, double sustain, double release)
    cdef void _dub(self, SoundBuffer sound, int framepos=*)
    cdef void _fill(self, double[:,:] frames)
    cdef double[:,:] _speed(self, double speed)

cdef class RingBuffer:
    cdef public int length
    cdef public int samplerate
    cdef public int channels
    cdef public int write_head
    cdef public double[:,:] frames
    cdef double[:,:] copyout

    cdef double[:,:] _read(self, int length, int offset)
    cdef void _write(self, double[:,:] frames)
