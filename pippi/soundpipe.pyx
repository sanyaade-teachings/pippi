from pippi.soundbuffer cimport SoundBuffer
from libc.stdlib cimport malloc, free
import numpy as np

cdef double[:,:] _butbp(double[:,:] snd, double[:,:] out, float freq, int length, int channels):
    cdef sp_data* sp
    cdef sp_butbp** butbps = <sp_butbp**>malloc(channels * sizeof(sp_butbp*))
    cdef float sample = 0
    cdef float output = 0
    cdef int i = 0
    cdef int c = 0

    sp_create(&sp)

    for c in range(channels):
        sp_butbp_create(&butbps[c])
        sp_butbp_init(sp, butbps[c])
        butbps[c].freq = freq

    for c in range(channels):
        for i in range(length):
            sample = <float>snd[i,c]
            sp_butbp_compute(sp, butbps[c], &sample, &output)
            out[i,c] = <double>output

    for c in range(channels):
        sp_butbp_destroy(&butbps[c])

    sp_destroy(&sp)
    free(butbps)

    return out

cpdef SoundBuffer butbp(SoundBuffer snd, float freq):
    cdef int length = len(snd)
    cdef int channels = snd.channels
    cdef double[:,:] out = np.zeros((length, channels), dtype='d')
    snd.frames = _butbp(snd.frames, out, freq, length, channels)
    return snd

cdef double[:,:] _butbr(double[:,:] snd, double[:,:] out, float freq, int length, int channels):
    cdef sp_data* sp
    cdef sp_butbr** butbrs = <sp_butbr**>malloc(channels * sizeof(sp_butbr*))
    cdef float sample = 0
    cdef float output = 0
    cdef int i = 0
    cdef int c = 0

    sp_create(&sp)

    for c in range(channels):
        sp_butbr_create(&butbrs[c])
        sp_butbr_init(sp, butbrs[c])
        butbrs[c].freq = freq

    for c in range(channels):
        for i in range(length):
            sample = <float>snd[i,c]
            sp_butbr_compute(sp, butbrs[c], &sample, &output)
            out[i,c] = <double>output

    for c in range(channels):
        sp_butbr_destroy(&butbrs[c])

    sp_destroy(&sp)
    free(butbrs)

    return out

cpdef SoundBuffer butbr(SoundBuffer snd, float freq):
    cdef int length = len(snd)
    cdef int channels = snd.channels
    cdef double[:,:] out = np.zeros((length, channels), dtype='d')
    snd.frames = _butbr(snd.frames, out, freq, length, channels)
    return snd

cdef double[:,:] _buthp(double[:,:] snd, double[:,:] out, float freq, int length, int channels):
    cdef sp_data* sp
    cdef sp_buthp** buthps = <sp_buthp**>malloc(channels * sizeof(sp_buthp*))
    cdef float sample = 0
    cdef float output = 0
    cdef int i = 0
    cdef int c = 0

    sp_create(&sp)

    for c in range(channels):
        sp_buthp_create(&buthps[c])
        sp_buthp_init(sp, buthps[c])
        buthps[c].freq = freq

    for c in range(channels):
        for i in range(length):
            sample = <float>snd[i,c]
            sp_buthp_compute(sp, buthps[c], &sample, &output)
            out[i,c] = <double>output

    for c in range(channels):
        sp_buthp_destroy(&buthps[c])

    sp_destroy(&sp)
    free(buthps)

    return out

cpdef SoundBuffer buthp(SoundBuffer snd, float freq):
    cdef int length = len(snd)
    cdef int channels = snd.channels
    cdef double[:,:] out = np.zeros((length, channels), dtype='d')
    snd.frames = _buthp(snd.frames, out, freq, length, channels)
    return snd

cdef double[:,:] _butlp(double[:,:] snd, double[:,:] out, float freq, int length, int channels):
    cdef sp_data* sp
    cdef sp_butlp** butlps = <sp_butlp**>malloc(channels * sizeof(sp_butlp*))
    cdef float sample = 0
    cdef float output = 0
    cdef int i = 0
    cdef int c = 0

    sp_create(&sp)

    for c in range(channels):
        sp_butlp_create(&butlps[c])
        sp_butlp_init(sp, butlps[c])
        butlps[c].freq = freq

    for c in range(channels):
        for i in range(length):
            sample = <float>snd[i,c]
            sp_butlp_compute(sp, butlps[c], &sample, &output)
            out[i,c] = <double>output

    for c in range(channels):
        sp_butlp_destroy(&butlps[c])

    sp_destroy(&sp)
    free(butlps)

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

