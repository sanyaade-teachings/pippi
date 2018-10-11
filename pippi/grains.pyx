# cython: language_level=3

from pippi.wavetables cimport HANN, PHASOR, to_window, to_wavetable
from pippi cimport interpolation
from pippi cimport soundpipe as SP
from libc.stdlib cimport rand, RAND_MAX, malloc, free
import numpy as np
from cpython cimport array
import array

DEF MIN_DENSITY = 0.000001
DEF MIN_SPEED = 0.000001
DEF MIN_GRAIN_FRAMELENGTH = 441
DEF DEFAULT_WTSIZE = 4096
DEF DEFAULT_GRAINLENGTH = 60
DEF DEFAULT_MAXGRAINLENGTH = 80
DEF DEFAULT_SPEED = 1
DEF DEFAULT_MAXSPEED = 2
DEF DEFAULT_DENSITY = 1
DEF DEFAULT_MINDENSITY = 0.5
DEF DEFAULT_MAXDENSITY = 1.5

cdef double** snd2array(double[:,:] snd, int length, int channels):
    cdef int i = 0
    cdef int c = 0
    cdef double** tbls = <double**>malloc(channels * sizeof(double*))
    cdef double* tbl

    for c in range(channels):
        tbl = <double*>malloc(length * sizeof(double))
        for i in range(length):
            tbl[i] = snd[i,c]
        tbls[c] = tbl

    return tbls

cdef class Cloud:
    def __cinit__(self, 
            double[:,:] snd, 
            object window=None, 
            object position=None,
            object amp=1.0,
            object speed=1.0, 
            object spread=0.0, 
            object jitter=0.0, 
            object grainlength=0.082, 
            object grid=None,
            object lpf=None,
            object hpf=None, 
            object bpf=None,
            object mask=None,
            unsigned int wtsize=4096,
            unsigned int samplerate=44100,
        ):

        self.snd = SP.memoryview2ftbls(snd)
        self.framelength = <unsigned int>len(snd)
        self.length = <double>(self.framelength / <double>samplerate)
        self.channels = <unsigned int>snd.shape[1]
        self.samplerate = samplerate

        self.wtsize = wtsize

        if window is None:
            window = HANN
        self.window = to_window(window)

        if position is None:
            position = PHASOR
        self.position = to_window(position)

        self.speed = to_wavetable(speed)
        self.spread = to_wavetable(spread)
        self.jitter = to_wavetable(jitter)
        self.grainlength = to_wavetable(grainlength)

        if grid is None:
            grid = np.multiply(self.grainlength, 0.5)
        self.grid = to_wavetable(grid)

        if lpf is None:
            self.has_lpf = False
        else:
            self.has_lpf = True
            self.lpf = to_wavetable(lpf)

        if hpf is None:
            self.has_hpf = False
        else:
            self.has_hpf = True
            self.hpf = to_wavetable(hpf)

        if bpf is None:
            self.has_bpf = False
        else:
            self.has_bpf = True
            self.bpf = to_wavetable(bpf)

        if mask is None:
            self.has_mask = False
        else:
            self.has_mask = True
            self.mask = array.array('i', mask)

    def __dealloc__(self):
        cdef int c = 0
        for c in self.channels:
            free(self.snd[c])
        free(self.snd)

    def play(self, double length):
        cdef unsigned int outframelength = <unsigned int>(self.samplerate * length)
        cdef double[:,:] out = np.zeros((outframelength, self.channels), dtype='d')
        cdef unsigned int write_pos = 0
        cdef unsigned int read_pos = 0
        cdef unsigned int grainlength = 4410
        cdef unsigned int grainincrement = grainlength // 2
        cdef double pos = 0
        cdef double grainpos = 0
        cdef double panpos = 0
        cdef double sample = 0
        cdef float fsample = 0
        cdef double spread = 0
        cdef unsigned int masklength = 0
        cdef unsigned int count = 0
        cdef SP.sp_data* sp
        cdef SP.sp_mincer* mincer
        cdef SP.sp_ftbl* tbl

        SP.sp_create(&sp)

        if self.has_mask:
            masklength = <unsigned int>len(self.mask)

        while write_pos < outframelength - grainlength: 
            if self.has_mask and self.mask[<int>(count % masklength)] == 0:
                write_pos += <unsigned int>(interpolation._linear_point(self.grid, pos) * self.samplerate)
                pos = <double>write_pos / <double>outframelength
                count += 1
                continue

            grainlength = <unsigned int>(interpolation._linear_point(self.grainlength, pos) * self.samplerate)
            read_pos = <unsigned int>(interpolation._linear_point(self.position, pos) * self.length)

            spread = interpolation._linear_point(self.spread, pos)
            panpos = (rand()/<double>RAND_MAX) * spread + (0.5 - (spread * 0.5))
            pans = [panpos, 1-panpos]

            for c in range(self.channels):
                SP.sp_ftbl_bind(sp, &tbl, self.snd[c], self.framelength)
                SP.sp_mincer_create(&mincer)
                SP.sp_mincer_init(sp, mincer, tbl, self.wtsize)

                panpos = pans[c % 2]

                for i in range(grainlength):
                    grainpos = <double>i / grainlength
                    if i+read_pos < self.length:
                        mincer.time = <float>((read_pos+i)/<float>self.samplerate)
                        mincer.pitch = <float>interpolation._linear_point(self.speed, pos)
                        mincer.amp = <float>interpolation._linear_point(self.amp, pos)

                        SP.sp_mincer_compute(sp, mincer, NULL, &fsample)
                        sample = <double>fsample * interpolation._linear_point(self.window, grainpos)
                        sample *= panpos
                        out[i+write_pos,c] += sample

                SP.sp_mincer_destroy(&mincer)
                SP.sp_ftbl_destroy(&tbl)

            write_pos += <unsigned int>(interpolation._linear_point(self.grid, pos) * self.samplerate)
            pos = <double>write_pos / <double>outframelength
            count += 1

        SP.sp_destroy(&sp)

        return out

"""
cdef double[:,:] _play(GrainCloud cloud, double[:,:] out):
    cdef int input_length = len(cloud.buf)
    cdef int framelength = out.shape[0]
    cdef int channels = out.shape[1]

    cdef int fi = 0
    cdef int ci = 0

    cdef int write_pos = 0 
    cdef int write_inc = 0 
    cdef int start = 0 
    cdef int end = 0 
    cdef double panpos = 0.5

    cdef double read_frac_pos = 0
    cdef double grain_speed = 0
    cdef double density_frac_pos = 0
    cdef double grainlength_frac_pos = 0

    cdef int initial_grainlength = <int>(<double>cloud.samplerate * cloud.grainlength * 0.001)
    cdef int grainlength = initial_grainlength

    cdef int preresample_grainlength = 0
    cdef int target_grainlength = 0

    cdef int max_read_pos = input_length - grainlength
    cdef double read_pos = 0
    cdef double[:,:] grain
    cdef double[:,:] grain_pre
    cdef double[:] grainchan
    cdef double[:] grainchan_pre

    cdef double playhead = 0
    
    while write_pos < framelength - grainlength:
        playhead = <double>write_pos / framelength

        # Set (target) grain length
        if cloud.has_grainlength_lfo:
            grainlength_frac_pos = interpolation._linear_point(cloud.grainlength_lfo, playhead)

            if cloud.jitter > 0:
                grainlength_frac_pos = grainlength_frac_pos * ((rand()/<double>RAND_MAX) * cloud.jitter + 1)

            grainlength = <int>(((grainlength_frac_pos * (cloud.maxlength - cloud.minlength)) + cloud.minlength) * (cloud.samplerate / 1000.0))
            max_read_pos = input_length - grainlength

        if grainlength < 0:
            grainlength *= -1

        # Get read position for _cut
        if cloud.freeze >= 0:
            read_pos = (cloud.samplerate * cloud.freeze)
            if read_pos > <double>max_read_pos:
                read_pos = <double>max_read_pos
            read_frac_pos = read_pos / <double>max_read_pos
        else:
            read_frac_pos = interpolation._linear_point(cloud.read_lfo, playhead)

        start = <int>(read_frac_pos * max_read_pos)


        # Get grain speed
        if cloud.has_speed_lfo:
            grain_speed = interpolation._linear_point(cloud.speed_lfo, playhead)
            cloud.speed = (grain_speed * (cloud.maxspeed - cloud.minspeed)) + cloud.minspeed


        ############################
        # Cut grain and pitch shift
        if cloud.speed != 1:
            # Length of segment to copy from source buffer: grainlength * speed
            #cloud.speed = cloud.speed if cloud.speed > cloud.minspeed else cloud.minspeed
            #cloud.speed = cloud.speed if cloud.speed < cloud.maxspeed else cloud.maxspeed
            preresample_grainlength = <int>(grainlength * cloud.speed)
            if preresample_grainlength > (framelength - start):
                # get min(available_length, preresample_length)
                # adjust target/grainlength to match speed based on available length
                preresample_grainlength = framelength - start
                cloud.speed = <double>preresample_grainlength / grainlength


            # TODO can probably init maxlength versions of these outside the loop, 
            # and reuse them in sequence (careful of gil-releasing...)
            grain_pre = np.zeros((preresample_grainlength, channels), dtype='d')
            grainchan_pre = np.zeros(preresample_grainlength, dtype='d')
            grain = np.zeros((grainlength, channels), dtype='d')
            grainchan = np.zeros(grainlength, dtype='d')

            grain_pre = _cut(cloud.buf.frames, framelength, start, preresample_grainlength)
            grain = _speed(grain_pre, grain, grainchan_pre, grainchan, channels)
        else:
            grain = np.zeros((grainlength, channels), dtype='d')
            grain = _cut(cloud.buf.frames, framelength, start, grainlength)


        # Apply grain window
        grain = _env(grain, channels, cloud.win)

        # Pan grain if spread > 0
        if cloud.spread > 0:
            panpos = (rand()/<double>RAND_MAX) * cloud.spread + (0.5 - (cloud.spread * 0.5))
            grain = _pan(grain, grainlength, channels, panpos, wts.CONSTANT)


        # Dub grain to output
        if write_pos + len(grain) < len(out):
            for fi in range(len(grain)):
                for ci in range(channels):
                    out[fi + write_pos, ci] += (grain[fi, ci] * cloud.amp)


        # Get density for write_inc modulation
        if cloud.has_density_lfo:
            density_frac_pos = interpolation._linear_point(cloud.density_lfo, playhead)
            density = (density_frac_pos * (cloud.maxdensity - cloud.mindensity)) + cloud.mindensity
        else:
            density = cloud.density

        # Increment write_pos based on grainlength & density
        try:
            cloud.grains_per_sec = <double>cloud.samplerate / (<double>len(grain) / 2.0)
            cloud.grains_per_sec *= density
            write_inc = <int>(<double>cloud.samplerate / cloud.grains_per_sec)
            if write_inc < MIN_GRAIN_FRAMELENGTH:
                write_pos += MIN_GRAIN_FRAMELENGTH
            else:
                write_pos += write_inc
        except ZeroDivisionError:
            write_pos += MIN_GRAIN_FRAMELENGTH
        
    return out


cdef class GrainCloud:
    def __init__(self, 
            SoundBuffer buf, 

            object win=None,
            int wtsize=DEFAULT_WTSIZE,

            list mask=None,

            double freeze=-1, 
            object read_lfo=None,
            double read_lfo_speed=1,

            object speed_lfo=None,
            double speed=DEFAULT_SPEED,
            double minspeed=DEFAULT_SPEED, 
            double maxspeed=DEFAULT_MAXSPEED, 

            object density_lfo=None,
            double density_lfo_speed=1,
            double density=DEFAULT_DENSITY, 
            double mindensity=DEFAULT_MINDENSITY, 
            double maxdensity=DEFAULT_MAXDENSITY, 

            double grainlength=DEFAULT_GRAINLENGTH, 
            object grainlength_lfo=None,
            double grainlength_lfo_speed=1,

            double minlength=DEFAULT_GRAINLENGTH,    # min grain length in ms
            double maxlength=DEFAULT_MAXGRAINLENGTH,    # max grain length in ms
            double spread=0,        # 0 = no panning, 1 = max random panning 
            double jitter=0,        # rhythm 0=regular, 1=totally random
            double amp=1, 
        ):

        if spread < 0:
            spread = 0
        elif spread > 1:
            spread = 1

        self.buf = buf
        self.channels = buf.channels
        self.samplerate = buf.samplerate
        self.spread = spread
        self.jitter = jitter
        self.freeze = freeze
        self.amp = amp
        self.wtsize = wtsize

        cdef array.array cmask
        if mask is not None:
            cmask = array.array('i', mask)
            self.mask = cmask
    
        # Window is always a wavetable, defaults to Hann
        if win is None:
            self.win = wts._window(wts.HANN, wtsize)
        else:
            self.win = wts.to_wavetable(win, wtsize)

        # Read lfo is always a wavetable, defaults to phasor
        if read_lfo is None:
            self.read_lfo = wts._window(wts.PHASOR, wtsize)
        else:
            self.read_lfo = wts.to_wavetable(read_lfo, wtsize)

        self.read_lfo_speed = read_lfo_speed

        self.has_speed_lfo = False
        if speed_lfo is not None:
            self.speed_lfo = wts.to_wavetable(speed_lfo, wtsize)
            self.has_speed_lfo = True

        if speed <= 0:
            speed = MIN_SPEED

        if minspeed <= 0:
            minspeed = MIN_SPEED

        if maxspeed <= 0:
            maxspeed = MIN_SPEED

        self.speed = speed
        self.minspeed = minspeed
        self.maxspeed = maxspeed

        self.density = density
        self.mindensity = mindensity
        self.maxdensity = maxdensity
        self.has_density_lfo = False
        if density_lfo is not None:
            self.density_lfo = wts.to_wavetable(density_lfo, wtsize)
            self.has_density_lfo = True

        self.density_lfo_speed = density_lfo_speed

        self.has_grainlength_lfo = False
        if grainlength_lfo is not None:
            self.grainlength_lfo = wts.to_wavetable(grainlength_lfo, wtsize)
            self.has_grainlength_lfo = True

        self.grainlength = grainlength
        self.minlength = minlength
        self.maxlength = maxlength
        self.grainlength_lfo_speed = grainlength_lfo_speed
 
    cpdef SoundBuffer play(GrainCloud self, double length=10):
        cdef int framelength = <int>(self.samplerate * length)
        cdef double[:,:] out = np.zeros((framelength, self.channels), dtype='d')
        out = _play(self, out)
        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

"""

