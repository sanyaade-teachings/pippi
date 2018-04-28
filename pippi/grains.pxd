from .soundbuffer cimport SoundBuffer
from . cimport wavetables

cdef class GrainCloud:
    cdef public SoundBuffer buf
    cdef public double amp
    cdef public int channels
    cdef public int samplerate
    cdef public double[:] win
    cdef public int win_length

    cdef public int[:] mask

    cdef public double freeze
    cdef public double[:] read_lfo
    cdef public int read_lfo_length
    cdef public double read_lfo_speed
    cdef public double read_jitter

    cdef public double[:] speed_lfo
    cdef public int speed_lfo_length
    cdef public double speed
    cdef public double minspeed
    cdef public double maxspeed

    cdef public double spread
    cdef public double jitter
    cdef public double density
    cdef public double mindensity
    cdef public double maxdensity
    cdef public double grains_per_sec

    cdef public double[:] density_lfo
    cdef public int density_lfo_length
    cdef public double density_lfo_speed
    cdef public double density_jitter

    cdef public double grainlength
    cdef public double[:] grainlength_lfo
    cdef public int grainlength_lfo_length
    cdef public double grainlength_lfo_speed
    cdef public double minlength
    cdef public double maxlength

