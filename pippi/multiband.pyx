#cython: language_level=3

from pippi cimport interpolation
from pippi cimport wavetables
from libc.stdlib cimport malloc, free
import numpy as np


cdef double[:,:] _extract_band(double[:,:] snd, double[:,:] out, double[:] minfreq, double[:] maxfreq):
    cdef sp_data* sp
    cdef sp_buthp* buthp1
    cdef sp_buthp* buthp2
    cdef sp_butlp* butlp1
    cdef sp_butlp* butlp2

    cdef int i = 0
    cdef int c = 0
    cdef double pos = 0

    cdef int length = len(snd)
    cdef int channels = snd.shape[1]

    cdef double sample = 0
    cdef double filtered = 0
    cdef double output = 0
    cdef double _minfreq, _maxfreq

    sp_create(&sp)

    for c in range(channels):
        sp_buthp_create(&buthp1)
        sp_buthp_create(&buthp2)
        sp_buthp_init(sp, buthp1)
        sp_buthp_init(sp, buthp2)

        sp_butlp_create(&butlp1)
        sp_butlp_create(&butlp2)
        sp_butlp_init(sp, butlp1)
        sp_butlp_init(sp, butlp2)

        for i in range(length):
            pos = <double>i / <double>length
            _minfreq = interpolation._linear_pos(minfreq, pos)
            _maxfreq = interpolation._linear_pos(maxfreq, pos)

            buthp1.freq = _maxfreq
            buthp2.freq = _maxfreq

            butlp1.freq = _minfreq
            butlp2.freq = _minfreq

            sample = <double>snd[i,c]

            if _maxfreq < 20000:
                sp_buthp_compute(sp, buthp1, &sample, &filtered)
                sp_buthp_compute(sp, buthp2, &filtered, &filtered)

            if _minfreq > 1:
                sp_butlp_compute(sp, butlp1, &filtered, &filtered)
                sp_butlp_compute(sp, butlp2, &filtered, &output)

            out[i,c] = <double>output

        sp_buthp_destroy(&buthp1)
        sp_buthp_destroy(&buthp2)

        sp_butlp_destroy(&butlp1)
        sp_butlp_destroy(&butlp2)

    sp_destroy(&sp)

    return out

cdef double mtof(double note):
    return 2**((note-69)/12.0) * 440.0

cpdef list split(SoundBuffer snd, double interval=3, list freqs=None, object drift=None, double driftwidth=0):
    cdef int length = len(snd)
    cdef int channels = snd.channels

    cdef list out = []
    cdef double[:,:] band = np.zeros((length, channels), dtype='d')

    cdef double[:] minfreq
    cdef double[:] maxfreq

    cdef double freq = 0

    minfreq = wavetables.to_window(0)
    maxfreq = wavetables.to_window(mtof(interval))
    band = _extract_band(snd.frames, band, minfreq, maxfreq)
    out += [ SoundBuffer(band.copy(), channels=channels, samplerate=snd.samplerate) ]

    cdef double note = interval

    while mtof(note+interval) < 20000:
        minfreq = wavetables.to_window(mtof(note))
        maxfreq = wavetables.to_window(mtof(note+interval))
        band = _extract_band(snd.frames, band, minfreq, maxfreq)
        out += [ SoundBuffer(band.copy(), channels=channels, samplerate=snd.samplerate) ]
        note += interval

    if mtof(note) < 20000:
        minfreq = wavetables.to_window(mtof(note))
        maxfreq = wavetables.to_window(20000)
        band = _extract_band(snd.frames, band, minfreq, maxfreq)
        out += [ SoundBuffer(band.copy(), channels=channels, samplerate=snd.samplerate) ]

    return out
