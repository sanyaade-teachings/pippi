from pippi.soundbuffer cimport SoundBuffer
import numpy as np

cdef double[:,:] _butbp(double[:,:] snd, double[:,:] out, float freq, int length, int channels):
    cdef sp_data* sp
    cdef sp_butbp* butbp
    cdef float sample = 0
    cdef float output = 0
    cdef int i = 0
    cdef int c = 0

    sp_create(&sp)
    sp_butbp_create(&butbp)
    sp_butbp_init(sp, butbp)

    butbp.freq = freq

    for i in range(length):
        for c in range(channels):
            sample = <float>snd[i,c]
            sp_butbp_compute(sp, butbp, &sample, &output)
            out[i,c] = <double>output

    sp_butbp_destroy(&butbp)
    sp_destroy(&sp)

    return out

cpdef SoundBuffer butbp(SoundBuffer snd, float freq):
    cdef int length = len(snd)
    cdef int channels = snd.channels
    cdef double[:,:] out = np.zeros((length, channels), dtype='d')
    snd.frames = _butbp(snd.frames, out, freq, length, channels)
    return snd

cdef double[:,:] _butbr(double[:,:] snd, double[:,:] out, float freq, int length, int channels):
    cdef sp_data* sp
    cdef sp_butbr* butbr
    cdef float sample = 0
    cdef float output = 0
    cdef int i = 0
    cdef int c = 0

    sp_create(&sp)
    sp_butbr_create(&butbr)
    sp_butbr_init(sp, butbr)

    butbr.freq = freq

    for i in range(length):
        for c in range(channels):
            sample = <float>snd[i,c]
            sp_butbr_compute(sp, butbr, &sample, &output)
            out[i,c] = <double>output

    sp_butbr_destroy(&butbr)
    sp_destroy(&sp)

    return out

cpdef SoundBuffer butbr(SoundBuffer snd, float freq):
    cdef int length = len(snd)
    cdef int channels = snd.channels
    cdef double[:,:] out = np.zeros((length, channels), dtype='d')
    snd.frames = _butbr(snd.frames, out, freq, length, channels)
    return snd

cdef double[:,:] _buthp(double[:,:] snd, double[:,:] out, float freq, int length, int channels):
    cdef sp_data* sp
    cdef sp_buthp* buthp
    cdef float sample = 0
    cdef float output = 0
    cdef int i = 0
    cdef int c = 0

    sp_create(&sp)
    sp_buthp_create(&buthp)
    sp_buthp_init(sp, buthp)

    buthp.freq = freq

    for i in range(length):
        for c in range(channels):
            sample = <float>snd[i,c]
            sp_buthp_compute(sp, buthp, &sample, &output)
            out[i,c] = <double>output

    sp_buthp_destroy(&buthp)
    sp_destroy(&sp)

    return out

cpdef SoundBuffer buthp(SoundBuffer snd, float freq):
    cdef int length = len(snd)
    cdef int channels = snd.channels
    cdef double[:,:] out = np.zeros((length, channels), dtype='d')
    snd.frames = _buthp(snd.frames, out, freq, length, channels)
    return snd

cdef double[:,:] _butlp(double[:,:] snd, double[:,:] out, float freq, int length, int channels):
    cdef sp_data* sp
    cdef sp_butlp* butlp
    cdef float sample = 0
    cdef float output = 0
    cdef int i = 0
    cdef int c = 0

    sp_create(&sp)
    sp_butlp_create(&butlp)
    sp_butlp_init(sp, butlp)

    butlp.freq = freq

    for i in range(length):
        for c in range(channels):
            sample = <float>snd[i,c]
            sp_butlp_compute(sp, butlp, &sample, &output)
            out[i,c] = <double>output

    sp_butlp_destroy(&butlp)
    sp_destroy(&sp)

    return out

cpdef SoundBuffer butlp(SoundBuffer snd, float freq):
    cdef int length = len(snd)
    cdef int channels = snd.channels
    cdef double[:,:] out = np.zeros((length, channels), dtype='d')
    snd.frames = _butlp(snd.frames, out, freq, length, channels)
    return snd

cdef double[:,:] _compressor(double[:,:] snd, double[:,:] out, float ratio, float thresh, float atk, float rel, int length, int channels):
    cdef sp_data* sp
    cdef sp_compressor* compressor
    cdef float sample = 0
    cdef float output = 0
    cdef int i = 0
    cdef int c = 0

    sp_create(&sp)
    sp_compressor_create(&compressor)
    sp_compressor_init(sp, compressor)

    compressor.ratio = &ratio
    compressor.thresh = &thresh
    compressor.atk = &atk
    compressor.rel = &rel

    for i in range(length):
        for c in range(channels):
            sample = <float>snd[i,c]
            sp_compressor_compute(sp, compressor, &sample, &output)
            out[i,c] = <double>output

    sp_compressor_destroy(&compressor)
    sp_destroy(&sp)

    return out

cpdef SoundBuffer compressor(SoundBuffer snd, float ratio, float thresh, float atk, float rel):
    cdef int length = len(snd)
    cdef int channels = snd.channels
    cdef double[:,:] out = np.zeros((length, channels), dtype='d')
    snd.frames = _compressor(snd.frames, out, ratio, thresh, atk, rel, length, channels)
    return snd

