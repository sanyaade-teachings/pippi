#clib soundpipe

from libc.stdint cimport uint32_t, int64_t
from .soundbuffer cimport SoundBuffer

cdef extern from "soundpipe.h":
    ctypedef struct sp_data:
        float* out;
        int sr
        int nchan
        unsigned long len
        unsigned long pos
        char filename[200]
        uint32_t rand

    int sp_create(sp_data**)
    int sp_destroy(sp_data**)
    int sp_process(sp_data*, void*, void (*callback)(sp_data*, void*))

    ctypedef struct sp_compressor:
        void* faust
        int argpos
        float* args[4]
        float* ratio
        float* thresh
        float* atk
        float* rel

    int sp_compressor_create(sp_compressor**)
    int sp_compressor_destroy(sp_compressor**)
    int sp_compressor_init(sp_data*, sp_compressor*)
    int sp_compressor_compute(sp_data*, sp_compressor*, float*, float*)

    ctypedef struct sp_butlp:
        float sr, freq, istor
        float lkf
        float a[8]
        float pidsr

    int sp_butlp_create(sp_butlp** p)
    int sp_butlp_destroy(sp_butlp** p)
    int sp_butlp_init(sp_data* sp, sp_butlp* p)
    int sp_butlp_compute(sp_data*, sp_butlp*, float*, float*)

    ctypedef struct sp_buthp:
        float sr, freq, istor
        float lkf
        float a[8]
        float pidsr

    int sp_buthp_create(sp_buthp** p)
    int sp_buthp_destroy(sp_buthp** p)
    int sp_buthp_init(sp_data* sp, sp_buthp* p)
    int sp_buthp_compute(sp_data*, sp_buthp*, float*, float*)

    ctypedef struct sp_butbp:
        float sr, freq, istor
        float lkf
        float a[8]
        float pidsr

    int sp_butbp_create(sp_butbp** p)
    int sp_butbp_destroy(sp_butbp** p)
    int sp_butbp_init(sp_data* sp, sp_butbp* p)
    int sp_butbp_compute(sp_data*, sp_butbp*, float*, float*)

    ctypedef struct sp_butbr:
        float sr, freq, istor
        float lkf
        float a[8]
        float pidsr

    int sp_butbr_create(sp_butbr** p)
    int sp_butbr_destroy(sp_butbr** p)
    int sp_butbr_init(sp_data* sp, sp_butbr* p)
    int sp_butbr_compute(sp_data*, sp_butbr*, float*, float*)

cdef double[:,:] _butbr(double[:,:] snd, double[:,:] out, float freq, int length, int channels)
cpdef SoundBuffer butbr(SoundBuffer snd, float freq)
cdef double[:,:] _butbp(double[:,:] snd, double[:,:] out, float freq, int length, int channels)
cpdef SoundBuffer butbp(SoundBuffer snd, float freq)
cdef double[:,:] _buthp(double[:,:] snd, double[:,:] out, float freq, int length, int channels)
cpdef SoundBuffer buthp(SoundBuffer snd, float freq)
cdef double[:,:] _butlp(double[:,:] snd, double[:,:] out, float freq, int length, int channels)
cpdef SoundBuffer butlp(SoundBuffer snd, float freq)

cdef double[:,:] _compressor(double[:,:] snd, double[:,:] out, float ratio, float thresh, float atk, float rel, int length, int channels)
cpdef SoundBuffer compressor(SoundBuffer snd, float ratio, float thresh, float atk, float rel)
