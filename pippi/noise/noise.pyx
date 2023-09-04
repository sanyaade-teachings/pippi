#cython: language_level=3

from pippi.defaults cimport DEFAULT_CHANNELS, DEFAULT_SAMPLERATE
from pippi.wavetables cimport to_wavetable, to_window, Wavetable
from pippi.interpolation cimport _linear_point, _linear_pos
from pippi.soundbuffer cimport SoundBuffer

cdef lpbuffer_t * to_lpbuffer_wavetable_from_wavetable(Wavetable wavetable):
    cdef lpbuffer_t * wt
    cdef int i, length

    length = len(wavetable.data)
    wt = LPBuffer.create(length, 1, 48000)

    for i in range(length):
        wt.data[i] = <lpfloat_t>wavetable.data[i]

    return wt

cdef lpbuffer_t * to_lpbuffer_wavetable_from_soundbuffer(SoundBuffer wavetable):
    cdef lpbuffer_t * wt
    cdef int i, length

    length = len(wavetable.frames)
    wt = LPBuffer.create(4096, 1, 48000)

    for i in range(length):
        wt.data[i] = <lpfloat_t>wavetable.frames[i,0]

    return wt

cdef int to_wavetable_flag(str wavetable_name=None):
    if wavetable_name == 'sine':
        return WT_SINE

    if wavetable_name == 'cos':
        return WT_COS

    if wavetable_name == 'tri':
        return WT_TRI

    if wavetable_name == 'phasor':
        return WT_RSAW

    if wavetable_name == 'rnd':
        return WT_RND

    if wavetable_name == 'saw':
        return WT_SAW

    if wavetable_name == 'rsaw':
        return WT_RSAW

    return WT_RND


cpdef SoundBuffer bln(object wt, double length, object minfreq, object maxfreq, channels=DEFAULT_CHANNELS, samplerate=DEFAULT_SAMPLERATE):
    cdef double[:] _minfreq = to_window(minfreq)
    cdef double[:] _maxfreq = to_window(maxfreq)
    cdef SoundBuffer out
    cdef size_t i, c, framelength
    cdef lpbuffer_t * _wt
    cdef lpblnosc_t * osc
    cdef lpfloat_t sample
    cdef double pos = 0

    if isinstance(wt, SoundBuffer):
        _wt = to_lpbuffer_wavetable_from_soundbuffer(wt)

    elif isinstance(wt, Wavetable):
        _wt = to_lpbuffer_wavetable_from_wavetable(wt)

    else:
        _wt = LPWavetable.create(to_wavetable_flag(wt), 512);

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

