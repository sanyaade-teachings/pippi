# cython: language_level=3

from .soundbuffer cimport SoundBuffer, _cut, _dub, _speed, _pan, _env
from . cimport wavetables
from . cimport interpolation
from libc.stdlib cimport rand, RAND_MAX
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

cdef double[:,:] _play(GrainCloud cloud, double[:,:] out, int framelength, double length):
    cdef int input_length = len(cloud.buf)

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

    cdef unsigned long initial_grainlength = <unsigned long>(<double>cloud.samplerate * cloud.grainlength * 0.001)
    cdef unsigned long grainlength = initial_grainlength

    cdef unsigned long preresample_grainlength = 0
    cdef unsigned long target_grainlength = 0

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
        if cloud.grainlength_lfo is not None:
            grainlength_frac_pos = interpolation._linear_point(cloud.grainlength_lfo, playhead)

            if cloud.jitter > 0:
                grainlength_frac_pos = grainlength_frac_pos * ((rand()/<double>RAND_MAX) * cloud.jitter + 1)

            grainlength = <int>(((grainlength_frac_pos * (cloud.maxlength - cloud.minlength)) + cloud.minlength) * (cloud.samplerate / 1000.0))
            max_read_pos = input_length - grainlength

        if grainlength < 0:
            print(grainlength, write_pos)
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
        if cloud.speed_lfo is not None:
            grain_speed = interpolation._linear_point(cloud.speed_lfo, playhead)
            cloud.speed = (grain_speed * (cloud.maxspeed - cloud.minspeed)) + cloud.minspeed


        ############################
        # Cut grain and pitch shift
        if cloud.speed != 1:
            # Length of segment to copy from source buffer: grainlength * speed
            #cloud.speed = cloud.speed if cloud.speed > cloud.minspeed else cloud.minspeed
            #cloud.speed = cloud.speed if cloud.speed < cloud.maxspeed else cloud.maxspeed
            preresample_grainlength = <unsigned long>(grainlength * cloud.speed)
            if preresample_grainlength > (framelength - start):
                # get min(available_length, preresample_length)
                # adjust target/grainlength to match speed based on available length
                preresample_grainlength = framelength - start
                cloud.speed = <double>preresample_grainlength / grainlength


            # TODO can probably init maxlength versions of these outside the loop, 
            # and reuse them in sequence (careful of gil-releasing...)
            grain_pre = np.zeros((preresample_grainlength, cloud.channels), dtype='d')
            grainchan_pre = np.zeros(preresample_grainlength, dtype='d')
            grain = np.zeros((grainlength, cloud.channels), dtype='d')
            grainchan = np.zeros(grainlength, dtype='d')

            grain_pre = _cut(cloud.buf.frames, framelength, start, preresample_grainlength)
            grain = _speed(grain_pre, grain, grainchan_pre, grainchan, cloud.channels)
        else:
            grain = np.zeros((grainlength, cloud.channels), dtype='d')
            grain = _cut(cloud.buf.frames, framelength, start, grainlength)


        # Apply grain window
        grain = _env(grain, cloud.channels, cloud.win)

        # Pan grain if spread > 0
        if cloud.spread > 0:
            panpos = (rand()/<double>RAND_MAX) * cloud.spread + (0.5 - (cloud.spread * 0.5))
            grain = _pan(grain, grainlength, cloud.channels, panpos, wavetables.CONSTANT)


        # Dub grain to output
        if write_pos + len(grain) < len(out):
            for fi in range(len(grain)):
                for ci in range(cloud.channels):
                    out[fi + write_pos, ci] += (grain[fi, ci] * cloud.amp)


        # Get density for write_inc modulation
        if cloud.density_lfo is not None:
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

            int win=-1,
            double[:] win_wt=None,
            int win_length=DEFAULT_WTSIZE,

            list mask=None,

            double freeze=-1, 
            int read_lfo=-1,
            object read_lfo_wt=None,
            double read_lfo_speed=1,
            unsigned int read_lfo_length=DEFAULT_WTSIZE,

            int speed_lfo=-1,
            double[:] speed_lfo_wt=None,
            unsigned int speed_lfo_length=DEFAULT_WTSIZE,
            double speed=DEFAULT_SPEED,
            double minspeed=DEFAULT_SPEED, 
            double maxspeed=DEFAULT_MAXSPEED, 

            int density_lfo=-1,
            double[:] density_lfo_wt=None,
            int density_lfo_length=DEFAULT_WTSIZE,
            double density_lfo_speed=1,
            double density=DEFAULT_DENSITY, 
            double mindensity=DEFAULT_MINDENSITY, 
            double maxdensity=DEFAULT_MAXDENSITY, 

            double grainlength=DEFAULT_GRAINLENGTH, 
            int grainlength_lfo=-1,
            double[:] grainlength_lfo_wt=None,
            unsigned int grainlength_lfo_length=DEFAULT_WTSIZE,
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

        cdef array.array cmask
        if mask is not None:
            cmask = array.array('i', mask)
            self.mask = cmask
    
        # Window is always a wavetable, defaults to Hann
        if win > -1:
            self.win = wavetables._window(win, win_length)
        elif win_wt is not None:
            self.win = win_wt
        else:
            self.win = wavetables._window(wavetables.HANN, win_length)
        self.win_length = len(self.win)

        # Read lfo is always a wavetable, defaults to phasor
        if read_lfo > -1:
            self.read_lfo = wavetables._window(read_lfo, read_lfo_length)
        elif read_lfo_wt is not None:
            self.read_lfo = interpolation._linear(np.asarray(read_lfo_wt, dtype='d'), read_lfo_length)
        else:
            self.read_lfo = wavetables._window(wavetables.PHASOR, read_lfo_length)

        self.read_lfo_speed = read_lfo_speed
        self.read_lfo_length = read_lfo_length

        # If speed_lfo < 0 and speed_lfo_wt is None, then use fixed speed
        # No transposition is done if speed == 1
        if speed_lfo > -1:
            self.speed_lfo = wavetables._window(speed_lfo, speed_lfo_length)
        elif speed_lfo_wt is not None:
            self.speed_lfo = speed_lfo_wt
        else:
            self.speed_lfo = None

        if self.speed_lfo is not None:
            self.speed_lfo_length = len(self.speed_lfo)

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
        if density_lfo > -1:
            self.density_lfo = wavetables._window(density_lfo, density_lfo_length)
        elif density_lfo_wt is not None:
            self.density_lfo = density_lfo_wt
        else:
            self.density_lfo = None

        self.density_lfo_speed = density_lfo_speed
        if self.density_lfo is not None:
            self.density_lfo_length = len(self.density_lfo)

        if grainlength_lfo > -1:
            self.grainlength_lfo = wavetables._window(grainlength_lfo, grainlength_lfo_length)
            self.grainlength = -1
        elif grainlength_lfo_wt is not None:
            self.grainlength_lfo = grainlength_lfo_wt
        else:
            self.grainlength_lfo = None

        self.grainlength = grainlength
        self.minlength = minlength
        self.maxlength = maxlength
        self.grainlength_lfo_speed = grainlength_lfo_speed
        if self.grainlength_lfo is not None:
            self.grainlength_lfo_length = len(self.grainlength_lfo)
 
    cpdef SoundBuffer play(GrainCloud self, double length=10):
        cdef int framelength = <int>(self.samplerate * length)
        cdef double[:,:] out = np.zeros((framelength, self.channels), dtype='d')
        out = _play(self, out, framelength, length)
        return SoundBuffer(out, length=length, channels=self.channels, samplerate=self.samplerate)



