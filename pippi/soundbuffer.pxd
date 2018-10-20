cdef double[:,:] sb_adsr(double[:,:] frames, int framelength, int channels, double samplerate, double attack, double decay, double sustain, double release) nogil
cdef double[:,:] _speed(double[:,:] frames, double[:,:] out, double[:] chan, double[:] outchan, int channels) nogil
cdef double[:,:] _dub(double[:,:] target, int target_length, double[:,:] todub, int todub_length, int channels, int framepos) nogil
cdef double[:,:] _cut(double[:,:] snd, int snd_length, int start, int length) nogil
cdef double[:,:] _pan(double[:,:] out, int length, int channels, double pos, int method)
cdef double[:,:] _env(double[:,:] snd, int channels, double[:] win) nogil

cdef class SoundBuffer:
    cdef public int samplerate
    cdef public int channels
    cdef public double[:,:] frames

    cdef void _dub(SoundBuffer self, SoundBuffer sound, int framepos)
    cdef void _fill(SoundBuffer self, double[:,:] frames)
    cpdef SoundBuffer adsr(SoundBuffer self, double a=*, double d=*, double s=*, double r=*)


