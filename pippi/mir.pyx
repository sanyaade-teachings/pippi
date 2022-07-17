#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport Wavetable

import aubio
import numpy as np
cimport numpy as np

try:
    # librosa depends on numba which is not supported at all on some platforms
    import librosa
except ImportError as e:
    print('Warning: could not load librosa. Please install librosa to use the feature extraction procedures in the mir module.')


cdef int DEFAULT_WINSIZE = 4096

np.import_array()

cpdef np.ndarray flatten(SoundBuffer snd):
    return np.asarray(snd.remix(1).frames, dtype='f').flatten()

cdef np.ndarray _bandwidth(np.ndarray snd, int samplerate, int winsize):
    return librosa.feature.spectral_bandwidth(y=snd, sr=samplerate, n_fft=winsize)

cpdef Wavetable bandwidth(SoundBuffer snd, int winsize=DEFAULT_WINSIZE):
    cdef np.ndarray wt = _bandwidth(flatten(snd), snd.samplerate, winsize)
    return Wavetable(wt.transpose().astype('d').flatten())

cdef np.ndarray _flatness(np.ndarray snd, int winsize):
    return librosa.feature.spectral_flatness(y=snd, n_fft=winsize)

cpdef Wavetable flatness(SoundBuffer snd, int winsize=DEFAULT_WINSIZE):
    cdef np.ndarray wt = _flatness(flatten(snd), winsize)
    return Wavetable(wt.transpose().astype('d').flatten())

cdef np.ndarray _rolloff(np.ndarray snd, int samplerate, int winsize):
    return librosa.feature.spectral_rolloff(y=snd, sr=samplerate, n_fft=winsize)

cpdef Wavetable rolloff(SoundBuffer snd, int winsize=DEFAULT_WINSIZE):
    cdef np.ndarray wt = _rolloff(flatten(snd), snd.samplerate, winsize)
    return Wavetable(wt.transpose().astype('d').flatten())

cdef np.ndarray _centroid(np.ndarray snd, int samplerate, int winsize):
    return librosa.feature.spectral_centroid(y=snd, sr=samplerate, n_fft=winsize)

cpdef Wavetable centroid(SoundBuffer snd, int winsize=DEFAULT_WINSIZE):
    cdef np.ndarray wt = _centroid(flatten(snd), snd.samplerate, winsize)
    return Wavetable(wt.transpose().astype('d').flatten())

cdef np.ndarray _contrast(np.ndarray snd, int samplerate, int winsize):
    return librosa.feature.spectral_contrast(y=snd, sr=samplerate, n_fft=winsize)

cpdef Wavetable contrast(SoundBuffer snd, int winsize=DEFAULT_WINSIZE):
    cdef np.ndarray wt = _contrast(flatten(snd), snd.samplerate, winsize)
    return Wavetable(wt.transpose().astype('d').flatten())

cpdef Wavetable pitch(SoundBuffer snd, double tolerance=0.8, str method=None, int winsize=DEFAULT_WINSIZE, bint backfill=True, double autotune=0, double fallback=220.):
    """ Returns a wavetable of non-zero frequencies detected which exceed the confidence threshold given. Frequencies are 
        held until the next detection to avoid zeros and outliers. Depending on the input, you may need to play with the 
        tolerance value and the window size to tune the behavior. The default detection method is `yinfast`. 

        Example:

            pitches = mir.pitch(snd, 0.8)

        * Yin implementation ported from:
        * Patrice Guyot. (2018, April 19). Fast Python 
        * implementation of the Yin algorithm (Version v1.1.1). 
        * Zenodo. http://doi.org/10.5281/zenodo.1220947
        * https://github.com/patriceguyot/Yin

        See libpippi/src/mir.c for implementation notes.
    """

    yin = LPPitchTracker.yin_create(4096, <int>snd.samplerate)
    yin.fallback = <lpfloat_t>fallback

    cdef list pitches = []
    cdef float[:] src = flatten(snd)

    cdef lpfloat_t s
    cdef lpfloat_t last_p = -1
    cdef lpfloat_t p = fallback

    for s in src:
        p = LPPitchTracker.yin_process(yin, s);
        if(p > 0 and p != last_p):
            last_p = p
            pitches += [ p ]

    if len(pitches) == 0:
        return None

    return Wavetable(pitches)

cpdef list onsets(SoundBuffer snd, str method=None, int winsize=DEFAULT_WINSIZE, bint seconds=True):
    """ Returns a list of onset times in seconds detected using the given method. The default method is `specflux`. 
        An optional `seconds` argument (normally true) may be set to false to return a list of frame indexes instead 
        of seconds.

        Example:

            onsets = mir.onsets(snd, 'specflux')

        Methods:

        This routine is a wrapper for [aubio](https://aubio.org)'s onset detection. 
        Summaries of the detection methods were taken from: https://aubio.org/manpages/latest/aubioonset.1.html
        Part of that man page was copied and slightly modified for formatting per the terms of the GPL license 
        which the original page was published under. Please see the above link for details.

        `hfc` - High-Frequency content 

        > This method computes the High Frequency Content (HFC) of the input spectral frame. 
        > The resulting function is efficient at detecting percussive onsets.
        >
        > Paul Masri. Computer modeling of Sound for Transformation and Synthesis of Musical Signal. 
        > PhD dissertation, University of Bristol, UK, 1996. 

        `complex` - Complex domain onset detection function 

        > This function uses information both in frequency and in phase to determine changes in 
        > the spectral content that might correspond to musical onsets. It is best suited for 
        > complex signals such as polyphonic recordings.
        >
        > Christopher Duxbury, Mike E. Davies, and Mark B. Sandler.
        > Complex domain onset detection for musical signals. In Proceedings of the Digital Audio 
        > Effects Conference, DAFx-03, pages 90-93, London, UK, 2003. 

        `phase` - Phase based onset detection function 

        > This function uses information both in frequency and in phase to determine changes 
        > in the spectral content that might correspond to musical onsets. It is best suited 
        > for complex signals such as polyphonic recordings.
        > 
        > Juan-Pablo Bello, Mike P. Davies, and Mark B. Sandler.
        > Phase-based note onset detection for music signals. In Proceedings of the IEEE International 
        > Conference on Acoustics Speech and Signal Processing, pages 441­444, Hong-Kong, 2003. 

        `specdiff` - Spectral difference onset detection function 

        > Jonhatan Foote and Shingo Uchihashi. The beat spectrum: a new approach to rhythm analysis. 
        > In IEEE International Conference on Multimedia and Expo (ICME 2001), pages 881­884, Tokyo, 
        > Japan, August 2001. 

        `kl` - Kulback-Liebler onset detection function 

        > Stephen Hainsworth and Malcom Macleod. Onset detection in music audio signals. In Proceedings 
        > of the International Computer Music Conference (ICMC), Singapore, 2003.

        `mkl` - Modified Kulback-Liebler onset detection function 

        > Paul Brossier, _Automatic annotation of musical audio for interactive systems_, Chapter 2, 
        > Temporal segmentation, PhD thesis, Centre for Digital music, Queen Mary University of London, 
        > London, UK, 2006.  

        `specflux` - Spectral flux 

        > Simon Dixon, Onset Detection Revisited, in ``Proceedings of the 9th International Conference 
        > on Digital Audio Effects'' (DAFx-06), Montreal, Canada, 2006.
    """
    if method is None:
        method = 'specflux'

    cdef int hopsize = winsize//2

    cdef int pos = 0
    cdef int total_length = len(snd)
    cdef int onset
    cdef int samplerate = snd.samplerate
    cdef list onsets = []

    o = aubio.onset(method, winsize, hopsize, samplerate)

    cdef float[:] chunk
    cdef float[:] src = flatten(snd)

    while pos < total_length - hopsize:
        chunk = src[pos:pos+hopsize]

        if o(np.asarray(chunk)):
            onset = o.get_last()
            onsets += [ onset ]

        pos += hopsize

    if seconds:
        onsets = [ onset / <double>samplerate for onset in onsets ]

    return onsets

cpdef list segments(SoundBuffer snd, str method=None, int winsize=DEFAULT_WINSIZE):
    """ A wrapper for `mir.onsets` which returns a list of SoundBuffers sliced at the onset times. 
        See the documentation for `mir.onsets` for an overview of the detection methods available.
    """
    cdef list onset_times = onsets(snd, method, winsize, seconds=False)
    cdef int last = -1
    cdef int onset = 0
    cdef list segments = []

    for onset in onset_times:
        if last < 0:
            last = onset
            continue

        segments += [ snd[last:onset] ]
        last = onset

    if last < len(snd):
        segments += [ snd[last:] ]

    return segments
