#cython: language_level=3

import numbers
import random
import math

cimport cython
import numpy as np
cimport numpy as np

from libc cimport math

from pippi cimport fft
from pippi.soundbuffer cimport SoundBuffer
from pippi cimport wavetables
from pippi.interpolation cimport _linear_point, _linear_pos, _bli_init, _bli_point, BLIData
from pippi.dsp cimport _mag
from pippi.defaults cimport PI
from pippi cimport soundpipe
from cpython cimport bool
from libc.stdlib cimport malloc, free


cdef double MINDENSITY = 0.001

cdef double[:,:] _crossfade(double[:,:] out, double[:,:] a, double[:,:] b, double[:] curve):
    cdef unsigned int length = len(a)
    cdef int channels = a.shape[1]
    cdef double pos = 0
    cdef double p = 0

    for i in range(length):
        pos = <double>i / <double>length
        p = _linear_pos(curve, pos)
        for c in range(channels):
            out[i,c] = (a[i,c] * p) + (b[i,c] * (1-p))

    return out

cpdef SoundBuffer crossfade(SoundBuffer a, SoundBuffer b, object curve=None):
    if len(a) != len(b):
        raise TypeError('Lengths do not match')
    if curve is None:
        curve = 'saw'
    cdef double[:] _curve = wavetables.Wavetable(curve, 0, 1).data
    cdef double[:,:] out = np.zeros((len(a), a.channels), dtype='d')
    return SoundBuffer(_crossfade(out, a.frames, b.frames, _curve), channels=a.channels, samplerate=a.samplerate)

cpdef SoundBuffer crush(SoundBuffer snd, object bitdepth=None, object samplerate=None):
    if bitdepth is None:
        bitdepth = random.triangular(0, 16)
    if samplerate is None:
        samplerate = random.triangular(0, snd.samplerate)
    cdef double[:] _bitdepth = wavetables.to_window(bitdepth)
    cdef double[:] _samplerate = wavetables.to_window(samplerate)
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    return SoundBuffer(soundpipe._bitcrush(snd.frames, out, _bitdepth, _samplerate, len(snd), snd.channels))

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double[:,:] _distort(double[:,:] snd, double[:,:] out):
    """ Non-linear distortion ported from supercollider """
    cdef int i=0, c=0
    cdef unsigned int framelength = len(snd)
    cdef int channels = snd.shape[1]
    cdef double s = 0

    for i in range(framelength):
        for c in range(channels):
            s = snd[i,c]
            if s > 0:
                out[i,c] = s / (1.0 + abs(s))
            else:
                out[i,c] = s / (1.0 - s)

    return out

cpdef SoundBuffer distort(SoundBuffer snd):
    """ Non-linear distortion ported from supercollider """
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    return SoundBuffer(_distort(snd.frames, out))

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double _blsc_integrated_clip(double val) nogil:
    cdef double out

    if val < -1:
        out = -4 / 5. * val - (1/3.0)
    else:
        out = (val * val) / 2.0 - val**6 / 30.0

    if val < 1:
        return out
    else:
        return 4./5.0 * val - 1./3.0

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double[:,:] _softclip(double[:,:] out, double[:,:] snd) noexcept nogil:
    cdef double val=0, lastval=0, sample=0
    cdef int c=0, i=0
    cdef int channels = snd.shape[1]
    cdef int length = len(snd)

    for c in range(channels):
        lastval = 0
        for i in range(length):
            val = snd[i,c]
            if abs(val - lastval) == 0.0:
                sample = (val + lastval)/2.0

                if sample < -1:
                    sample = -4. / 5.
                else:
                    sample = sample - sample**5 / 5.
                
                if sample >= 1:
                    sample = 4. / 5.

            else:
                sample = (_blsc_integrated_clip(val) - _blsc_integrated_clip(lastval)) / (val - lastval)

            out[i,c] = sample
            lastval = val

    return out

cpdef SoundBuffer softclip(SoundBuffer snd):
    """ Zener diode clipping simulation implemented by Will Mitchell of Starling Labs
    """
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    return SoundBuffer(_softclip(out, snd.frames), channels=snd.channels, samplerate=snd.samplerate)

cpdef SoundBuffer softclip2(SoundBuffer snd):
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    cdef lpfxsoftclip_t * clipper

    for c in range(snd.channels):
        clipper = LPSoftClip.create()

        for i in range(len(snd)):
            out[i,c] = LPSoftClip.process(clipper, snd.frames[i,c])

        LPSoftClip.destroy(clipper)
   
    return SoundBuffer(out, channels=snd.channels, samplerate=snd.samplerate)


@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double[:,:] _crossover(double[:,:] snd, double[:,:] out, double[:] amount, double[:] smooth, double[:] fade):
    """ Crossover distortion ported from the supercollider CrossoverDistortion ugen """
    cdef int i=0, c=0
    cdef unsigned int framelength = len(snd)
    cdef int channels = snd.shape[1]
    cdef double s=0, pos=0, a=0, f=0, m=0

    for i in range(framelength):
        pos = <double>i / <double>framelength
        a = _linear_pos(amount, pos)
        m = _linear_pos(smooth, pos)
        f = _linear_pos(fade, pos)

        for c in range(channels):
            s = abs(snd[i,c]) - a
            if s < 0:
                s *= (1.0 + (s * f)) * m

            if snd[i,c] < 0:
                s *= -1

            out[i,c] = s

    return out

cpdef SoundBuffer crossover(SoundBuffer snd, object amount, object smooth, object fade):
    """ Crossover distortion ported from the supercollider CrossoverDistortion ugen """
    cdef double[:] _amount = wavetables.to_window(amount)
    cdef double[:] _smooth = wavetables.to_window(smooth)
    cdef double[:] _fade = wavetables.to_window(fade)
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    return SoundBuffer(_crossover(snd.frames, out, _amount, _smooth, _fade))

cdef double _fold_point(double sample, double last, double samplerate):
    # Adapted from https://ccrma.stanford.edu/~jatin/ComplexNonlinearities/Wavefolder.html
    cdef double z = math.tanh(sample) + (math.tanh(last) * 0.9)
    return z + (-0.5 * math.sin(2 * PI * sample * (samplerate/2) / samplerate))

cdef double[:,:] _fold(double[:,:] out, double[:,:] snd, double[:] amp, double samplerate):
    cdef double last=0, sample=0, pos=0
    cdef int length = len(snd)
    cdef int channels = snd.shape[1]

    for c in range(channels):
        last = 0
        for i in range(length):
            pos = <double>i / length
            sample = snd[i,c] * _linear_pos(amp, pos)
            sample = _fold_point(sample, last, samplerate)
            last = sample
            out[i,c] = lpzapgremlins(sample)

    return out

cpdef SoundBuffer fold(SoundBuffer snd, object amp=1, bint norm=True):
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    cdef double[:] _amp = wavetables.to_window(amp)
    out = _fold(out, snd.frames, _amp, <double>snd.samplerate)
    if norm:
        out = _norm(out, snd.mag)
    return SoundBuffer(out, channels=snd.channels, samplerate=snd.samplerate)


@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double[:,:] _norm(double[:,:] snd, double ceiling):
    cdef long i = 0
    cdef int c = 0
    cdef long framelength = len(snd)
    cdef int channels = snd.shape[1]
    cdef double normval = 1
    cdef double maxval = _mag(snd)

    if maxval == 0:
        return snd

    normval = ceiling / maxval
    for i in range(framelength):
        for c in range(channels):
            snd[i,c] *= normval

    return snd

cpdef SoundBuffer norm(SoundBuffer snd, double ceiling):
    snd.frames = _norm(snd.frames, ceiling)
    return snd

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double[:,:] _vspeed(double[:,:] snd, double[:] chan, double[:,:] out, double[:] lfo, double minspeed, double maxspeed, int samplerate):
    cdef int i = 0
    cdef int c = 0
    cdef int framelength = len(snd)
    cdef int channels = snd.shape[1]
    cdef double speed = 0
    cdef double posinc = 1.0 / <double>framelength
    cdef double pos = 0
    cdef double lfopos = 0

    for c in range(channels):
        for i in range(framelength):
            chan[i] = snd[i,c]

        pos = 0
        lfopos = 0
        for i in range(framelength):
            speed = _linear_point(lfo, lfopos) * (maxspeed - minspeed) + minspeed
            out[i,c] = _linear_point(chan, pos)
            pos += posinc * speed
            lfopos += posinc

    return out

cpdef SoundBuffer vspeed(SoundBuffer snd, object lfo, double minspeed, double maxspeed):
    cdef double[:] _lfo = wavetables.to_wavetable(lfo)
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    cdef double[:] chan = np.zeros(len(snd), dtype='d')
    snd.frames = _vspeed(snd.frames, chan, out, _lfo, minspeed, maxspeed, snd.samplerate)
    return snd

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double[:,:] _widen(double[:,:] snd, double[:,:] out, double[:] width):
    cdef double mid, w, pos
    cdef int channels = snd.shape[1]
    cdef int length = len(snd)
    cdef int i, c, d=0

    for i in range(length-1):
        pos = <double>i / length
        w = _linear_pos(width, pos)
        mid = (1.0-w) / (1.0 + w)
        for c in range(channels):
            d = c + 1
            while d > channels:
                d -= channels
            out[i,c] = snd[i+1,c] + mid * snd[i+1,d]

    return out

cpdef SoundBuffer widen(SoundBuffer snd, object width=1):
    cdef double[:] _width = wavetables.to_window(width)
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    return SoundBuffer(_widen(snd.frames, out, _width), samplerate=snd.samplerate, channels=snd.channels)

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double[:,:] _delay(double[:,:] snd, double[:,:] out, int delayframes, double feedback):
    cdef int i = 0
    cdef int c = 0
    cdef int framelength = len(snd)
    cdef int channels = snd.shape[1]
    cdef int delayindex = 0
    cdef double sample = 0

    for i in range(framelength):
        delayindex = i - delayframes
        for c in range(channels):
            if delayindex < 0:
                sample = snd[i,c]
            else:
                sample = snd[delayindex,c] * feedback
                snd[i,c] += sample
            out[i,c] = sample

    return out

cpdef SoundBuffer delay(SoundBuffer snd, double delaytime, double feedback):
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    cdef int delayframes = <int>(snd.samplerate * delaytime)
    snd.frames = _delay(snd.frames, out, delayframes, feedback)
    return snd


@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(False)
cdef double[:,:] _vdelay(double[:,:] snd, 
                         double[:,:] out, 
                         double[:] lfo, 
                         double[:,:] delayline, 
                         double mindelay, 
                         double maxdelay, 
                         double feedback, 
                         int samplerate):
    cdef int i = 0
    cdef int c = 0
    cdef double pos = 0
    cdef int framelength = len(snd)
    cdef int delaylinelength = len(delayline)
    cdef int channels = snd.shape[1]
    cdef int delayindex = 0
    cdef int delayindexnext = 0
    cdef int delaylineindex = 0
    cdef double sample = 0
    cdef double output = 0
    cdef double delaytime = 0
    cdef int delayframes = 0

    """
        double interp_delay(double n, double buffer[], int L, current) {
            int t1, t2;
            t1 = current + n;
            t1 %= L
            t2 = t1 + 1
            t2 %= L

            return buffer[t1] + (n - <int>n) * (buffer[t2] - buffer[t1]);
        }

        t1 = i + delaytimeframes
        t1 %= delaylinelength
        t2 = t1 + 1
        t2 %= delaylinelength

    for i in range(framelength):
        pos = <double>i / <double>framelength
        delaytime = (_linear_point(lfo, pos) * (maxdelay-mindelay) + mindelay) * samplerate
        delayindex = delaylineindex + <int>delaytime
        delayindex %= delaylinelength
        delayindexnext = delayindex + 1
        delayindexnext %= delaylinelength

        for c in range(channels):
            delayline[delaylineindex,c] += snd[i,c] * feedback
            sample = delayline[delayindex,c] + ((delaytime - <int>delaytime) * (delayline[delayindexnext,c] + delayline[delayindex,c]))
            out[i,c] = sample + snd[i,c]

        delaylineindex -= 1
        delaylineindex %= delaylinelength
        #print(delaylineindex, delayindex)
    """

    delayindex = 0

    for i in range(framelength):
        pos = <double>i / <double>framelength
        delaytime = (_linear_point(lfo, pos) * (maxdelay-mindelay) + mindelay) * samplerate
        delayreadindex = <int>(i - delaytime)
        for c in range(channels):
            sample = snd[i,c]

            if delayreadindex >= 0:
                output = snd[delayreadindex,c] * feedback
                sample += output

            delayindex += 1
            delayindex %= delaylinelength

            delayline[delayindex,c] = output

            out[i,c] = sample

    return out

cpdef SoundBuffer vdelay(SoundBuffer snd, object lfo, double mindelay, double maxdelay, double feedback):
    cdef double[:] lfo_wt = wavetables.to_wavetable(lfo, 4096)
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    cdef int maxdelayframes = <int>(snd.samplerate * maxdelay)
    cdef double[:,:] delayline = np.zeros((maxdelayframes, snd.channels), dtype='d')
    snd.frames = _vdelay(snd.frames, out, lfo_wt, delayline, mindelay, maxdelay, feedback, snd.samplerate)
    return snd


@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double[:,:] _mdelay(double[:,:] snd, double[:,:] out, int[:] delays, double feedback):
    cdef int i = 0
    cdef int c = 0
    cdef int j = 0
    cdef int framelength = len(snd)
    cdef int numdelays = len(delays)
    cdef int channels = snd.shape[1]
    cdef int delayindex = 0
    cdef double sample = 0
    cdef double dsample = 0
    cdef double output = 0
    cdef int delaylinestart = 0
    cdef int delaylinepos = 0
    cdef int delayreadindex = 0

    cdef int delaylineslength = sum(delays)
    cdef double[:,:] delaylines = np.zeros((delaylineslength, channels), dtype='d')
    cdef int[:] delaylineindexes = np.zeros(numdelays, dtype='i')

    for i in range(framelength):
        for c in range(channels):
            sample = snd[i,c]
            delaylinestart = 0
            for j in range(numdelays):
                delayreadindex = i - delays[j]
                delayindex = delaylineindexes[j]
                delaylinepos = delaylinestart + delayindex
                output = delaylines[delaylinepos,c]

                if delayreadindex < 0:
                    dsample = 0
                else:
                    dsample = snd[delayreadindex,c] * feedback
                    output += dsample
                    sample += output

                delayindex += 1
                delayindex %= delays[j]

                delaylines[delaylinepos,c] = output
                delaylineindexes[j] = delayindex
                delaylinestart += delays[j]

            out[i,c] = sample

    return out

cpdef SoundBuffer mdelay(SoundBuffer snd, list delays, double feedback):
    cdef double[:,:] out = np.zeros((len(snd), snd.channels), dtype='d')
    cdef int numdelays = len(delays)
    cdef double delay
    cdef int[:] delayframes = np.array([ snd.samplerate * delay for delay in delays ], dtype='i')
    snd.frames = _mdelay(snd.frames, out, delayframes, feedback)
    return snd


@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
cdef double[:,:] _fir(double[:,:] snd, double[:,:] out, double[:] impulse, bint norm=True):
    cdef int i=0, c=0, j=0
    cdef int framelength = len(snd)
    cdef int channels = snd.shape[1]
    cdef int impulselength = len(impulse)
    cdef double maxval     

    if norm:
        maxval = _mag(snd)

    for i in range(framelength):
        for c in range(channels):
            for j in range(impulselength):
                out[i+j,c] += snd[i,c] * impulse[j]

    if norm:
        return _norm(out, maxval)
    else:
        return out

cpdef SoundBuffer fir(SoundBuffer snd, object impulse, bint normalize=True):
    cdef double[:] impulsewt = wavetables.to_window(impulse)
    cdef double[:,:] out = np.zeros((len(snd)+len(impulsewt)-1, snd.channels), dtype='d')
    return SoundBuffer(_fir(snd.frames, out, impulsewt, normalize), channels=snd.channels, samplerate=snd.samplerate)

cpdef Wavetable envelope_follower(SoundBuffer snd, double window=0.015):
    cdef int blocksize = <int>(window * snd.samplerate)
    cdef int length = len(snd)
    cdef int barrier = length - blocksize
    cdef double[:] flat = np.ravel(np.array(snd.remix(1).frames, dtype='d'))
    cdef double val = 0
    cdef int i, j, ei = 0
    cdef int numblocks = <int>(length / blocksize)
    cdef double[:] env = np.zeros(numblocks, dtype='d')

    while i < barrier:
        val = 0
        for j in range(blocksize):
            val = max(val, abs(flat[i+j]))

        env[ei] = val

        i += blocksize
        ei += 1

    return Wavetable(env)

# Soundpipe butterworth filters
cpdef SoundBuffer buttlpf(SoundBuffer snd, object freq):
    cdef double[:] _freq = wavetables.to_window(freq)
    return SoundBuffer(soundpipe.butlp(snd.frames, _freq), channels=snd.channels, samplerate=snd.samplerate)

cpdef SoundBuffer butthpf(SoundBuffer snd, object freq):
    cdef double[:] _freq = wavetables.to_window(freq)
    return SoundBuffer(soundpipe.buthp(snd.frames, _freq), channels=snd.channels, samplerate=snd.samplerate)

cpdef SoundBuffer buttbpf(SoundBuffer snd, object freq):
    cdef double[:] _freq = wavetables.to_window(freq)
    return SoundBuffer(soundpipe.butbp(snd.frames, _freq), channels=snd.channels, samplerate=snd.samplerate)

cpdef SoundBuffer brf(SoundBuffer snd, object freq):
    # TODO: rename to buttbrf after adding SVF band reject filter
    cdef double[:] _freq = wavetables.to_window(freq)
    return SoundBuffer(soundpipe.butbr(snd.frames, _freq), channels=snd.channels, samplerate=snd.samplerate)

# More Soundpipe FX
cpdef SoundBuffer compressor(SoundBuffer snd, double ratio=4, double threshold=-30, double attack=0.2, double release=0.2):
    return SoundBuffer(soundpipe.compressor(snd.frames, ratio, threshold, attack, release), channels=snd.channels, samplerate=snd.samplerate)

cpdef SoundBuffer saturator(SoundBuffer snd, double drive=10, double offset=0, bint dcblock=True):
    return SoundBuffer(soundpipe.saturator(snd.frames, drive, offset, dcblock), channels=snd.channels, samplerate=snd.samplerate)

cpdef SoundBuffer paulstretch(SoundBuffer snd, stretch=1, windowsize=1):
    return SoundBuffer(soundpipe.paulstretch(snd.frames, windowsize, stretch, snd.samplerate), channels=snd.channels, samplerate=snd.samplerate)

cpdef SoundBuffer mincer(SoundBuffer snd, double length, object position, object pitch, double amp=1, int wtsize=4096):
    cdef double[:] time_lfo = wavetables.to_window(position)
    cdef double[:] pitch_lfo = wavetables.to_window(pitch)
    return SoundBuffer(soundpipe.mincer(snd.frames, length, time_lfo, amp, pitch_lfo, wtsize, snd.samplerate), channels=snd.channels, samplerate=snd.samplerate)

# Convolutions
cdef double[:,:] _to_impulse(object impulse, int channels):
    cdef double[:,:] _impulse
    cdef double[:] _wt

    if isinstance(impulse, str):
        _wt = wavetables.to_window(impulse)
        _impulse = np.hstack([ _wt for _ in range(channels) ])

    elif isinstance(impulse, Wavetable):
        _impulse = np.hstack([ np.reshape(impulse.data, (-1, 1)) for _ in range(channels) ])

    elif isinstance(impulse, SoundBuffer):
        if impulse.channels != channels:
            _impulse = impulse.remix(channels).frames
        else:
            _impulse = impulse.frames

    else:
        raise TypeError('Could not convolve impulse of type %s' % type(impulse))

    return _impulse

cpdef SoundBuffer convolve(SoundBuffer snd, object impulse, bint norm=True):
    cdef double[:,:] _impulse = _to_impulse(impulse, snd.channels)
    cdef double[:,:] out = fft.conv(snd.frames, _impulse)

    if norm:
        out = _norm(out, snd.mag)

    return SoundBuffer(out, channels=snd.channels, samplerate=snd.samplerate)

cpdef SoundBuffer tconvolve(SoundBuffer snd, object impulse, bool normalize=True):
    cdef double[:,:] _impulse = _to_impulse(impulse, snd.channels)

    cdef int i=0, c=0, j=0
    cdef int framelength = len(snd)
    cdef int impulselength = len(_impulse)
    cdef double maxval     

    cdef double[:,:] out = np.zeros((framelength + impulselength, snd.channels), dtype='d')

    if normalize:
        maxval = _mag(snd.frames)

    for i in range(framelength):
        for c in range(snd.channels):
            for j in range(impulselength):
                out[i+j,c] += snd.frames[i,c] * _impulse[j,c]

    if normalize:
        out = _norm(out, maxval)

    return SoundBuffer(out, channels=snd.channels, samplerate=snd.samplerate)

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.initializedcheck(False)
@cython.cdivision(True)
cpdef SoundBuffer wconvolve(SoundBuffer snd, SoundBuffer impulse, object wsize=None, object grid=None, bool normalize=True):
    if wsize is None:
        wsize = 0.02

    if grid is None:
        grid = 'phasor'

    cdef double[:] _wsize = wavetables.to_window(wsize)
    cdef double[:] _grid = wavetables.to_window(grid)

    cdef double[:,:] _impulse = _to_impulse(impulse, snd.channels)

    cdef int i=0, c=0, j=0
    cdef int framelength = len(snd)
    cdef int impulselength = len(_impulse)
    cdef int windowlength 
    cdef double w = 1
    cdef int offset = 0
    cdef double maxval     
    cdef double g = 0
    cdef int outlength = framelength + impulselength

    cdef double[:,:] out = np.zeros((outlength, snd.channels), dtype='d')

    if normalize:
        maxval = _mag(snd.frames)

    with nogil:
        for i in range(framelength):
            pos = <double>i/<double>framelength
            g = _linear_pos(_grid, pos)
            w = _linear_pos(_wsize, pos)
            windowlength = max(16, <int>(impulselength * w))
            offset = <int>(g * (impulselength-windowlength))
            for c in range(snd.channels):
                for j in range(windowlength):
                    out[i+j,c] += snd.frames[i,c] * _impulse[j+offset,c]

    if normalize:
        out = _norm(out, maxval)

    return SoundBuffer(out, channels=snd.channels, samplerate=snd.samplerate)

cpdef SoundBuffer go(SoundBuffer snd, 
                          object factor,
                          double density=1, 
                          double wet=1,
                          double minlength=0.01, 
                          double maxlength=0.06, 
                          double minclip=0.4, 
                          double maxclip=0.8, 
                          object win=None
                    ):
    if wet <= 0:
        return snd

    cdef wavetables.Wavetable factors = None
    if not isinstance(factor, numbers.Real):
        factors = wavetables.Wavetable(factor)

    density = max(MINDENSITY, density)

    cdef double outlen = snd.dur + maxlength
    cdef SoundBuffer out = SoundBuffer(length=outlen, channels=snd.channels, samplerate=snd.samplerate)
    cdef wavetables.Wavetable window
    if win is None:
        window = wavetables.Wavetable(wavetables.HANN)
    else:
        window = wavetables.Wavetable(win)

    cdef double grainlength = random.triangular(minlength, maxlength)
    cdef double pos = 0
    cdef double clip
    cdef SoundBuffer grain

    while pos < outlen:
        grain = snd.cut(pos, grainlength)
        grain *= random.triangular(0, factor * wet)
        grain = grain.softclip()
        out.dub(grain * window.data, pos)

        pos += (grainlength/2) * (1/density)
        grainlength = random.triangular(minlength, maxlength)

    if wet > 0:
        out *= wet

    if wet < 1:
        out.dub(snd * abs(wet-1), 0)


    return out

# 2nd order state variable filter cookbook adapted from google ipython notebook
# https://github.com/google/music-synthesizer-for-android/blob/master/lab/Second%20order%20sections%20in%20matrix%20form.ipynb
# trapezoidal integration from Andrew Simper http://www.cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
cdef bint _is_2d_window(object item):
    cdef bint all_numbers = True
    if np.asarray(item).ndim > 1:
        return True
    if isinstance(item, list):
        for thing in item:
            if not (isinstance(thing, int) or isinstance(thing, float)):
                all_numbers = False
        return not all_numbers

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef void _svf_core(SVFData* data) noexcept nogil:
    
    data.res = max(min(data.res, 1), 0)
    cdef double g = math.tan(PI * data.freq) * data.shelf
    cdef double k = 2. - 2. * data.res

    cdef double a1 = 1. / (1. + g * (g + k))
    cdef double a2 = g * a1
    cdef double a3 = g * a2

    data.Az[0] = 2. * a1 - 1.
    data.Az[1] = -2. * a2
    data.Az[2] = 2. * a2
    data.Az[3] = 1. - 2. * a3

    data.Bz[0] = 2. * a2
    data.Bz[1] = 2. * a3

    cdef double[3] C_v0 
    # initializing by indexing allows nogil
    C_v0[0] = 1
    C_v0[1] = 0
    C_v0[2] = 0

    cdef double[3] C_v1
    C_v1[0] = a2
    C_v1[1] = a1
    C_v1[2] = -a2

    cdef double[3] C_v2
    C_v2[0] = a3
    C_v2[1] = a2
    C_v2[2] = 1 - a3

    data.Cz[0] = C_v0[0] * data.M[0] + C_v1[0] * data.M[1]  + C_v2[0] * data.M[2]
    data.Cz[1] = C_v0[1] * data.M[0] + C_v1[1] * data.M[1]  + C_v2[1] * data.M[2]
    data.Cz[2] = C_v0[2] * data.M[0] + C_v1[2] * data.M[1]  + C_v2[2] * data.M[2]

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.initializedcheck(False)
cdef void _svf_lp(SVFData* data):
    data.M = [0, 0, 1]
    data.shelf = 1
    _svf_core(data) 

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.initializedcheck(False)
cdef void _svf_bp(SVFData* data):
    data.M = [0, 1, 0]
    data.shelf = 1
    _svf_core(data)

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.initializedcheck(False)
cdef void _svf_hp(SVFData* data):
    cdef double k = 2 - 2 * data.res
    data.M = [1, -k, -1]
    data.shelf = 1
    _svf_core(data)

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.initializedcheck(False)
cdef void _svf_notch(SVFData* data):
    cdef double k = 2 - 2 * data.res
    data.M = [1, -k, 0]
    data.shelf = 1
    _svf_core(data)

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.initializedcheck(False)
cdef void _svf_peak(SVFData* data):
    cdef double k = 2 - 2 * data.res
    data.M = [1, -k, -2]
    data.shelf = 1
    _svf_core(data)

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef void _svf_bell(SVFData* data):
    cdef double A = 10 ** (data.gain / 40.)
    cdef double k = 1./(data.res * A)
    data.res = 1 - 0.5 * k
    data.M = [1, k * (A * A - 1), 0]
    data.shelf = 1
    _svf_core(data)

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef void _svf_lshelf(SVFData* data):
    cdef double A = 10 ** (data.gain / 40.)
    cdef double k = 1./(data.res)
    data.res = 1 - 0.5 * k
    data.M = [1, k * (A - 1), A * A - 1]
    data.shelf = 1/math.sqrt(A)
    _svf_core(data)

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef void _svf_hshelf(SVFData* data):
    cdef double A = 10 ** (data.gain / 40.)
    cdef double A2 = A * A
    cdef double k = 1./(data.res)
    data.res = 1 - 0.5 * k
    data.M = [A2, k * (A - A2), 1 - A2]
    data.shelf = math.sqrt(A)
    _svf_core(data)

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef SoundBuffer _svf(svf_filter_t mode, SoundBuffer snd, object freq, object res, object gain, bint norm):
    if freq is None:
        freq = 100
    if res is None:
        res = 0
    if gain is None:
        gain = 0

    cdef int channels = snd.channels
    cdef int length = len(snd)
    cdef int samplerate = snd.samplerate
    cdef double[:,:] frames = snd.frames

    cdef int freq_ch = 1
    cdef double[:,:] _freq
    window = []
    if _is_2d_window(freq):
        for channel in freq:
            window.append(wavetables.to_window(channel))
        _freq = np.asarray(window)
        freq_ch = len(freq)

    else:
        _freq = np.asarray([wavetables.to_window(freq)])
    
    cdef int res_ch = 1
    cdef double[:,:] _res
    window = []
    if _is_2d_window(res):
        for channel in res:
            window.append(wavetables.to_window(channel))
        _res = np.asarray(window)
        res_ch = len(res)
    else:
        _res = np.asarray([wavetables.to_window(res)])

    cdef int gain_ch = 1
    cdef double[:,:] _gain
    window = []
    if _is_2d_window(gain):
        for channel in gain:
            window.append(wavetables.to_window(channel))
        _gain = np.asarray(window)
        gain_ch = len(gain)
    else:
        _gain = np.asarray([wavetables.to_window(gain)])
        
    cdef int c = 0
    cdef int i = 0
    cdef double sample
    cdef double pos = 0
    cdef int _freq_ch = 0
    cdef int _res_ch = 0
    cdef int _gain_ch = 0

    cdef SVFData* data = <SVFData*>malloc(sizeof(SVFData))
    cdef double[:,:] out = np.zeros((length, channels), dtype='d')

    for c in range(channels):
        
        data.X = [0, 0]

        _freq_ch = c % freq_ch
        _res_ch = c % res_ch
        _gain_ch = c % gain_ch

        for i in range(length):
            pos = <double>i / length
            data.freq = max(min(_linear_pos(_freq[ _freq_ch], pos) / samplerate, 0.49), -.49)
            data.res = _linear_pos(_res[_res_ch], pos)
            data.gain = _linear_pos(_gain[_gain_ch], pos)
            sample = frames[i,c]

            mode(data)

            out[i,c] = sample * data.Cz[0] + data.X[0] * data.Cz[1] + data.X[1] * data.Cz[2]
            data.X[0] = sample * data.Bz[0] + data.X[0] * data.Az[0] + data.X[1] * data.Az[1]
            data.X[1] = sample * data.Bz[1] + data.X[0] * data.Az[2] + data.X[1] * data.Az[3]

    if norm:
        out = _norm(out, snd.mag)

    free(data)

    return SoundBuffer(out, channels=channels, samplerate=samplerate)   

cpdef SoundBuffer hpf(SoundBuffer snd, object freq=None, object res=None, bint norm=True):
    cdef svf_filter_t mode = _svf_hp
    return _svf(mode, snd, freq, res, None, norm)

cpdef SoundBuffer lpf(SoundBuffer snd, object freq=None, object res=None, bint norm=True):
    cdef svf_filter_t mode = _svf_lp
    return _svf(mode, snd, freq, res, None, norm)

cpdef SoundBuffer bpf(SoundBuffer snd, object freq=None, object res=None, bint norm=True):
    cdef svf_filter_t mode = _svf_bp
    return _svf(mode, snd, freq, res, None, norm)

cpdef SoundBuffer notchf(SoundBuffer snd, object freq=None, object res=None, bint norm=True):
    cdef svf_filter_t mode = _svf_notch
    return _svf(mode, snd, freq, res, None, norm)

cpdef SoundBuffer peakf(SoundBuffer snd, object freq=None, object res=None, bint norm=True):
    cdef svf_filter_t mode = _svf_peak
    return _svf(mode, snd, freq, res, None, norm)

cpdef SoundBuffer belleq(SoundBuffer snd, object freq=None, object q=None, object gain=None, bint norm=True):
    cdef svf_filter_t mode = _svf_bell
    return _svf(mode, snd, freq, q, gain, norm)

cpdef SoundBuffer lshelfeq(SoundBuffer snd, object freq=None, object q=None, object gain=None, bint norm=True):
    cdef svf_filter_t mode = _svf_lshelf
    return _svf(mode, snd, freq, q, gain, norm)

cpdef SoundBuffer hshelfeq(SoundBuffer snd, object freq=None, object q=None, object gain=None, bint norm=True):
    cdef svf_filter_t mode = _svf_hshelf
    return _svf(mode, snd, freq, q, gain, norm)

# mid/side encoding/decoding

cdef double _m_encode(double left, double right):
    return (left + right) * .707

cdef double _s_encode(double left, double right):
    return (left - right) * .707 

cdef double _l_decode(double mid, double side):
    return (mid + side) * .707

cdef double _r_decode(double mid, double side):
    return (mid - side) * .707

cpdef SoundBuffer ms_encode(SoundBuffer snd):
    snd = snd.remix(2)
    cdef int length = len(snd)
    cdef double[:,:] out = np.zeros((length, 2), dtype='d')
    cdef double[:,:] frames = snd.frames
    for i in range(length):
        out[i, 0] = _m_encode(snd.frames[i, 0], snd.frames[i, 1])
        out[i, 1] = _s_encode(snd.frames[i, 0], snd.frames[i, 1])
    return SoundBuffer(out, channels=2, samplerate=snd.samplerate)

cpdef SoundBuffer ms_decode(SoundBuffer snd):
    snd = snd.remix(2)
    cdef int length = len(snd)
    cdef double[:,:] out = np.zeros((length, 2), dtype='d')
    cdef double[:,:] frames = snd.frames
    for i in range(length):
        out[i, 0] = _l_decode(snd.frames[i, 0], snd.frames[i, 1])
        out[i, 1] = _r_decode(snd.frames[i, 0], snd.frames[i, 1])
    return SoundBuffer(out, channels=2, samplerate=snd.samplerate)

# Efficient power of two up/downsamplers
# From Fredrick Harris Multirate Signal Processing for Communication Systems
# Original paper with AG Constantinides
# https://www.researchgate.net/publication/259753999_Digital_Signal_Processing_with_Efficient_Polyphase_Recursive_All-pass_Filters
# Think homespun and less optimized version of Laurent De Soras HIIR
# Nothing fancy w/r/t group delay compensation, maybe someday

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef double _processHBAP1(HBAP* state, double sample):
    cdef double out = (sample - state.d2) * state.a0 + state.d1
    state.d1 = sample
    state.d2 = out
    return out

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef double _processHBAP2(HBAP* state, double sample):
    cdef double out1 = (sample - state.d2) * state.a0 + state.d1
    cdef double out2 = (out1 - state.d3) * state.a1 + state.d2
    state.d1 = sample
    state.d2 = out1
    state.d3 = out2
    return out2

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef void _initHBAP(HBAP* state):
    state.d1 = 0
    state.d2 = 0
    state.d3 = 0

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef SoundBuffer _decimate(SoundBuffer snd, int factor):

    # limit to defined range 1-5
    if (factor <= 0):
        return snd

    # create intermediary buffers for successive stages
    cdef double** stages = <double**>malloc(sizeof(double*) * factor)
    cdef int array_size = 2

    # create a buffer of allpass paths, 2 per stage
    cdef HBAP** filters = <HBAP**>malloc(sizeof(HBAP*) * factor * 2)

    cdef int i, j

    for i in range(factor):
        stages[i] = <double*>malloc(sizeof(double) * array_size)
        array_size *= 2
        if i < 2:
            filters[i * 2] = <HBAP*>malloc(sizeof(HBAP))
            filters[i * 2].a0 = 0.0798664262025582
            filters[i * 2].a1 = 0.5453236511825826
            filters[i * 2].process = _processHBAP2

            filters[i * 2 + 1] = <HBAP*>malloc(sizeof(HBAP))
            filters[i * 2 + 1].a0 = 0.283829344898100
            filters[i * 2 + 1].a1 = 0.834411891201724
            filters[i * 2 + 1].process = _processHBAP2
        else:
            filters[i * 2] = <HBAP*>malloc(sizeof(HBAP))
            filters[i * 2].a0 = 0.11192
            filters[i * 2].process = _processHBAP1

            filters[i * 2 + 1] = <HBAP*>malloc(sizeof(HBAP))
            filters[i * 2 + 1].a0 = 0.53976
            filters[i * 2 + 1].process = _processHBAP1

        _initHBAP(filters[i * 2])
        _initHBAP(filters[i * 2 + 1])


    cdef int oversample = <int>(2**factor)

    cdef int length_error = int(len(snd)) % oversample
    cdef int pad_size = (oversample-length_error)

    cdef int new_length = len(snd)//oversample
    snd = snd.pad(end=(pad_size + 1), samples=True)

    cdef int channels = snd.channels

    cdef double[:,:] out = np.zeros((new_length, channels), dtype='d')
    cdef double[:,:] frames = snd.frames

    cdef int c, m, n, stage
    cdef int buflength = oversample//2

    # for each channel
    for c in range(channels):
        # for each sample in the smaller decimated buffer
        for i in range(new_length):
            buflength = oversample//2
            # grab the samples to decimate (wish this could be wrapped into the first decimation stage better)
            for j in range(oversample):
                stages[factor - 1][j] = frames[i * oversample + j][c]
            # perform successive decimation by 2 stages
            stage = factor - 1
            while stage > 0:
                for n in range(buflength):
                    stages[stage - 1][n] = (filters[stage * 2].process(filters[stage * 2], stages[stage][n * 2 + 1]) + \
                        filters[stage * 2 + 1].process(filters[stage * 2 + 1], stages[stage][n * 2])) * .5 
                buflength //= 2
                stage -= 1

            out[i][c] = (filters[0].process(filters[0], stages[0][1]) + filters[1].process(filters[1], stages[0][0])) * .5 

    for i in range(factor):
        free(stages[i])
        free(filters[i*2])
        free(filters[i*2 + 1])
    free(filters)
    free(stages)

    return SoundBuffer(out, channels=snd.channels, samplerate=(snd.samplerate / oversample))

cpdef SoundBuffer decimate(SoundBuffer snd, int factor):
    return _decimate(snd, factor)


@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef SoundBuffer _upsample(SoundBuffer snd, int factor):

    # same setup as decimator
    if (factor <= 0):
        return snd
    cdef double** stages = <double**>malloc(sizeof(double*) * factor)
    cdef int array_size = 2
    cdef HBAP** filters = <HBAP**>malloc(sizeof(HBAP*) * factor * 2)
    cdef int i, j
    for i in range(factor):
        stages[i] = <double*>malloc(sizeof(double) * array_size)
        array_size *= 2
        if i < 2:
            filters[i * 2] = <HBAP*>malloc(sizeof(HBAP))
            filters[i * 2].a0 = 0.0798664262025582
            filters[i * 2].a1 = 0.5453236511825826
            filters[i * 2].process = _processHBAP2

            filters[i * 2 + 1] = <HBAP*>malloc(sizeof(HBAP))
            filters[i * 2 + 1].a0 = 0.283829344898100
            filters[i * 2 + 1].a1 = 0.834411891201724
            filters[i * 2 + 1].process = _processHBAP2
        else:
            filters[i * 2] = <HBAP*>malloc(sizeof(HBAP))
            filters[i * 2].a0 = 0.11192
            filters[i * 2].process = _processHBAP1

            filters[i * 2 + 1] = <HBAP*>malloc(sizeof(HBAP))
            filters[i * 2 + 1].a0 = 0.53976
            filters[i * 2 + 1].process = _processHBAP1
        _initHBAP(filters[i * 2])
        _initHBAP(filters[i * 2 + 1])
    
    cdef int oversample = <int>(2**factor)
    # no need to pad
    cdef int old_length = len(snd)
    cdef int new_length = old_length * oversample
    cdef int channels = snd.channels
    cdef double[:,:] out = np.zeros((new_length, channels), dtype='d')
    cdef double[:,:] frames = snd.frames
    cdef int c, m, n, stage, buflength

    # transpose of the decimator
    for c in range(channels):
        # for each sample in the smaller source buffer
        for i in range(old_length):
            # do the first stage of upsampling
            stages[0][0] = filters[0].process(filters[0], frames[i][c])
            stages[0][1] = filters[1].process(filters[1], frames[i][c])
            # perform successive upsampling by 2 stages
            stage = 1
            buflength = 2
            while stage < factor:
                for n in range(buflength):
                    stages[stage][2 * n] = filters[stage * 2].process(filters[stage * 2], stages[stage - 1][n])
                    stages[stage][2 * n + 1] = filters[stage * 2 + 1].process(filters[stage * 2 + 1], stages[stage - 1][n])
                buflength *= 2
                stage += 1
            # copy upsampled buffer
            for j in range(oversample):
                out[i * oversample + j][c] = stages[factor - 1][j]

    for i in range(factor):
        free(stages[i])
        free(filters[i*2])
        free(filters[i*2 + 1])
    free(filters)
    free(stages)

    return SoundBuffer(out, channels=snd.channels, samplerate=(snd.samplerate * oversample))

cpdef SoundBuffer upsample(SoundBuffer snd, int factor):
    return _upsample(snd, factor)

# arbitrary resampler
@cython.boundscheck(False)
@cython.wraparound(False)
@cython.cdivision(True)
@cython.initializedcheck(False)
cdef SoundBuffer _resample(SoundBuffer snd, double ratio, int quality, bint resample):
    if (ratio == 0) or (ratio is None) or (snd is None): return snd

    cdef double factor = 1.0/ratio
    cdef int new_length = <int>(factor * len(snd))

    cdef double changesr = 1.0
    if resample: changesr = factor

    if quality < 0: quality = 0
    cdef BLIData* bl_data = _bli_init(quality, False)
    bl_data.table_length = len(snd)
    bl_data.resampling_factor=factor

    cdef double[:,:] out = np.zeros((new_length, snd.channels), dtype='d')
    cdef double[:,:] frames = snd.frames.T

    cdef int c, i
    cdef double position
    for c in range(snd.channels):
        position = 0
        for i in range(new_length):
            out[i][c] = _bli_point(frames[c], position, bl_data)
            position += ratio

    return SoundBuffer(out, channels=snd.channels, samplerate=(snd.samplerate*changesr))

cpdef SoundBuffer resample(SoundBuffer snd, double ratio, int quality=5):
    return _resample(snd, ratio, quality, True)

cpdef SoundBuffer repitch(SoundBuffer snd, double ratio, int quality=5):
    return _resample(snd, ratio, quality, False)

cdef SoundBuffer _vspeed2(SoundBuffer snd, object speed, int quality, bint normalize):
    if (speed == 0) or (speed is None) or (snd is None): return snd

    cdef int channels = snd.channels
    cdef int length = len(snd)
    cdef int samplerate = snd.samplerate

    cdef double[:] _speed = wavetables.to_window(speed)
    cdef double speed_average = 0
    cdef int i
    if len(_speed) > 1:
        for i in range(1, len(_speed)):
            speed_average += (_speed[i] + _speed[i - 1]) * .5
        speed_average /= (len(_speed) - 1)
        if speed_average == 0: speed_average = .5

    else:
        speed_average = abs(_speed[0])
    
    cdef double factor = 1.0/abs(speed_average)
    cdef double normalizer
    if normalize: normalizer = abs(speed_average)
    else: normalizer = 1
    cdef int new_length = <int>(factor * length * normalizer)

    cdef double speed_inc = (len(_speed) - 1) / new_length

    if quality < 0: quality = 0
    cdef BLIData* bl_data = _bli_init(quality, False)
    bl_data.table_length = length
    bl_data.resampling_factor = 1

    cdef double[:,:] out = np.zeros((new_length, snd.channels), dtype='d')
    cdef double[:,:] frames = snd.frames.T

    cdef int c
    cdef double position, speed_pos, inc
    for c in range(snd.channels):
        position = 0
        speed_pos = 0
        for i in range(new_length):
            out[i][c] = _bli_point(frames[c], position, bl_data)
            speed_pos += speed_inc
            inc = _linear_point(_speed, speed_pos)
            position += inc / normalizer
            while position < 0:
                position += length
            while position >= length:
                position -= length
            if abs(inc) < 1:
                bl_data.resampling_factor = 1
            else:
                bl_data.resampling_factor = abs(1/inc)

    return SoundBuffer(out, channels=snd.channels, samplerate=snd.samplerate)

cpdef SoundBuffer vspeed2(SoundBuffer snd, object speed, int quality=5, bint normalize=False):
    return _vspeed2(snd, speed, quality, normalize)


cpdef SoundBuffer weave(SoundBuffer snd, double threshold=0.1, bint above=True):
    # Only output samples above or below the threshold
    # Inspired by the Wovenland 2 recordings by Toshiya Tsunoda & Taku Unami

    cdef long length = len(snd)
    cdef int channels = snd.channels
    cdef int samplerate = snd.samplerate

    cdef double[:,:] out = np.zeros((length, channels), dtype='d')

    cdef long i = 0
    cdef long c = 0

    if above:
        for c in range(channels):
            for i in range(length):
                if abs(snd.frames[i,c]) >= threshold:
                    out[i,c] = snd.frames[i,c]

    else:
        for c in range(channels):
            for i in range(length):
                if abs(snd.frames[i,c]) <= threshold:
                    out[i,c] = snd.frames[i,c]

    return SoundBuffer(out, channels=channels, samplerate=samplerate)

