#cython: language_level=3

from pippi.defaults cimport DEFAULT_CHANNELS, DEFAULT_SAMPLERATE
from pippi.wavetables cimport to_wavetable, to_window
from pippi.interpolation cimport _linear_point, _linear_pos
from pippi.soundbuffer cimport SoundBuffer

cpdef SoundBuffer bln(object wt, double length, object minfreq, object maxfreq, channels=DEFAULT_CHANNELS, samplerate=DEFAULT_SAMPLERATE):
    cdef double[:] _minfreq = to_window(minfreq)
    cdef double[:] _maxfreq = to_window(maxfreq)
    cdef SoundBuffer out
    cdef size_t i, c, framelength
    cdef lpbuffer_t * _wt
    cdef lpblnosc_t * osc
    cdef lpfloat_t sample
    cdef double pos = 0

    # FIXME when libpipppi wavetables are fully integrated, 
    # build the wavetable from the object passed into bln() again
    _wt = LPWavetable.create(WT_SINE, 512);
    out = SoundBuffer(length=length, channels=channels, samplerate=samplerate)

    osc = LPBLNOsc.create(_wt, _minfreq[0], _maxfreq[0]);
    osc.samplerate = samplerate

    framelength = <size_t>(length * samplerate)

    for i in range(framelength):
        pos = <double>i / <double>framelength
        osc.minfreq = <lpfloat_t>_linear_pos(_minfreq, pos)
        osc.maxfreq = <lpfloat_t>_linear_pos(_maxfreq, pos)

        sample = LPBLNOsc.process(osc)
        for c in range(channels):
            out.frames[i, c] = sample

    LPBLNOsc.destroy(osc);
    LPBuffer.destroy(_wt);

    return out

