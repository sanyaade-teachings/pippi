#cython: language_level=3

from pippi.wavetables cimport Wavetable
from pippi.soundbuffer cimport SoundBuffer

cdef double[:,:] _norm(double[:,:] snd, double ceiling)
cpdef SoundBuffer norm(SoundBuffer snd, double ceiling)

cdef double[:,:] _fir(double[:,:] snd, double[:,:] out, double[:] impulse, bint norm=*)
cpdef SoundBuffer fir(SoundBuffer snd, object impulse, bint normalize=*)

cdef double _fold_point(double sample, double last, double samplerate)
cdef double[:,:] _fold(double[:,:] out, double[:,:] snd, double[:] amp, double samplerate)
cpdef SoundBuffer fold(SoundBuffer snd, object amp=*, bint norm=*)

cpdef Wavetable envelope_follower(SoundBuffer snd, double window=*)
cpdef SoundBuffer widen(SoundBuffer snd, object width=*)

cdef double[:,:] _softclip(double[:,:] out, double[:,:] snd) nogil
cpdef SoundBuffer softclip(SoundBuffer snd)
cpdef SoundBuffer softclip2(SoundBuffer snd)

ctypedef void (*svf_filter_t)(SVFData* data)

ctypedef struct SVFData:
    double[4] Az 
    double[2] Bz
    double[3] Cz
    double[2] X

    double[3] M
    double freq
    double res
    double gain
    double shelf

cpdef SoundBuffer hpf(SoundBuffer snd, object freq=*, object res=*, bint norm=*)
cpdef SoundBuffer lpf(SoundBuffer snd, object freq=*, object res=*, bint norm=*)
cpdef SoundBuffer bpf(SoundBuffer snd, object freq=*, object res=*, bint norm=*)
cpdef SoundBuffer notchf(SoundBuffer snd, object freq=*, object res=*, bint norm=*)
cpdef SoundBuffer peakf(SoundBuffer snd, object freq=*, object res=*, bint norm=*)

cpdef SoundBuffer belleq(SoundBuffer snd, object freq=*, object q=*, object gain=*, bint norm=*)
cpdef SoundBuffer lshelfeq(SoundBuffer snd, object freq=*, object q=*, object gain=*, bint norm=*)
cpdef SoundBuffer hshelfeq(SoundBuffer snd, object freq=*, object q=*, object gain=*, bint norm=*)

cpdef SoundBuffer buttlpf(SoundBuffer snd, object freq)
cpdef SoundBuffer butthpf(SoundBuffer snd, object freq)
cpdef SoundBuffer buttbpf(SoundBuffer snd, object freq)
cpdef SoundBuffer brf(SoundBuffer snd, object freq)

ctypedef double (*HBAPProcess)(HBAP* data, double sample)

ctypedef struct HBAP:
    double d1
    double d2
    double d3
    double a0
    double a1	
    HBAPProcess process

cdef extern from "pippicore.h":
    ctypedef double lpfloat_t
    ctypedef struct lpbuffer_t:
        size_t length;
        int samplerate;
        int channels;
        lpfloat_t phase
        size_t boundry
        size_t range
        size_t pos
        size_t onset
        int is_looping
        lpfloat_t data[]

    cdef lpfloat_t lpzapgremlins(lpfloat_t x)
    ctypedef struct lpfx_factory_t:
        lpfloat_t (*read_skewed_buffer)(lpfloat_t freq, lpbuffer_t * buf, lpfloat_t phase, lpfloat_t skew)
        lpfloat_t (*lpf1)(lpfloat_t x, lpfloat_t * y, lpfloat_t cutoff, lpfloat_t samplerate)
        lpfloat_t (*hpf1)(lpfloat_t x, lpfloat_t * y, lpfloat_t cutoff, lpfloat_t samplerate)
        void (*convolve)(lpbuffer_t * a, lpbuffer_t * b, lpbuffer_t * out)
        void (*norm)(lpbuffer_t * buf, lpfloat_t ceiling)
        lpfloat_t (*crossover)(lpfloat_t val, lpfloat_t amount, lpfloat_t smooth, lpfloat_t fade)
        lpfloat_t (*fold)(lpfloat_t val, lpfloat_t * prev, lpfloat_t samplerate)
        lpfloat_t (*limit)(lpfloat_t val, lpfloat_t * prev, lpfloat_t threshold, lpfloat_t release, lpbuffer_t * d)
        lpfloat_t (*crush)(lpfloat_t val, int bits)

    extern const lpfx_factory_t LPFX

cdef extern from "fx.softclip.h":
    ctypedef struct lpfxsoftclip_t:
        lpfloat_t lastval

    ctypedef struct lpfxsoftclip_factory_t:
        lpfxsoftclip_t * (*create)()
        lpfloat_t (*process)(lpfxsoftclip_t * sc, lpfloat_t val)
        void (*destroy)(lpfxsoftclip_t * sc)

    extern const lpfxsoftclip_factory_t LPSoftClip

