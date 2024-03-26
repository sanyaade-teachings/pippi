#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport Wavetable
cimport numpy as np

cdef extern from "pippicore.h":
    ctypedef double lpfloat_t

    ctypedef struct lpbuffer_t:
        size_t length
        int samplerate
        int channels
        lpfloat_t phase
        size_t boundry
        size_t range
        size_t pos
        size_t onset
        int is_looping
        lpfloat_t data[]

cdef extern from "mir.h":
    ctypedef struct lpyin_t:
        lpbuffer_t * block
        int samplerate
        int blocksize
        int stepsize
        lpfloat_t last_pitch
        lpfloat_t threshold
        lpfloat_t fallback
        int offset
        int elapsed

        lpbuffer_t * tmp

        int tau_max
        int tau_min

    ctypedef struct lpmir_pitch_factory_t:
        lpyin_t * (*yin_create)(int, int)
        lpfloat_t (*yin_process)(lpyin_t *, lpfloat_t)
        void (*yin_destroy)(lpyin_t *)

    extern const lpmir_pitch_factory_t LPPitchTracker


cdef int DEFAULT_WINSIZE

cpdef np.ndarray flatten(SoundBuffer snd)

cdef np.ndarray _bandwidth(np.ndarray snd, int samplerate, int winsize)
cpdef Wavetable bandwidth(SoundBuffer snd, int winsize=*)

cdef np.ndarray _flatness(np.ndarray snd, int winsize)
cpdef Wavetable flatness(SoundBuffer snd, int winsize=*)

cdef np.ndarray _rolloff(np.ndarray snd, int samplerate, int winsize)
cpdef Wavetable rolloff(SoundBuffer snd, int winsize=*)

cdef np.ndarray _centroid(np.ndarray snd, int samplerate, int winsize)
cpdef Wavetable centroid(SoundBuffer snd, int winsize=*)

cdef np.ndarray _contrast(np.ndarray snd, int samplerate, int winsize)
cpdef Wavetable contrast(SoundBuffer snd, int winsize=*)

cpdef Wavetable pitch(SoundBuffer snd, double tolerance=*, str method=*, int winsize=*, bint backfill=*, double autotune=*, double fallback=*)

cpdef list onsets(SoundBuffer snd, str method=*, int winsize=*, bint seconds=*)
cpdef list segments(SoundBuffer snd, str method=*, int winsize=*)


