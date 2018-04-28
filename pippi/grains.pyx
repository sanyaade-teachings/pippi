from .soundbuffer cimport SoundBuffer
from . cimport wavetables
from . cimport interpolation
from libc.stdlib cimport rand, RAND_MAX
import numpy as np
from cpython cimport array
import array

DEF MIN_DENSITY = 0.000001
DEF MIN_SPEED = 0.000001
DEF MIN_GRAIN_FRAMELENGTH = 8
DEF DEFAULT_WTSIZE = 4096
DEF DEFAULT_GRAINLENGTH = 60
DEF DEFAULT_MAXGRAINLENGTH = 80
DEF DEFAULT_SPEED = 1
DEF DEFAULT_MAXSPEED = 2
DEF DEFAULT_DENSITY = 1
DEF DEFAULT_MINDENSITY = 0.5
DEF DEFAULT_MAXDENSITY = 1.5

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
            int read_lfo_length=DEFAULT_WTSIZE,

            int speed_lfo=-1,
            double[:] speed_lfo_wt=None,
            int speed_lfo_length=DEFAULT_WTSIZE,
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
            int grainlength_lfo_length=DEFAULT_WTSIZE,
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
        if self.freeze > -1:
            self.read_lfo = None
        elif read_lfo > -1:
            self.read_lfo = wavetables._window(read_lfo, read_lfo_length)
        elif read_lfo_wt is not None:
            #self.read_lfo = read_lfo_wt
            self.read_lfo = interpolation._linear(np.asarray(read_lfo_wt, dtype='d'), read_lfo_length)
        else:
            self.read_lfo = wavetables._window(wavetables.PHASOR, read_lfo_length)

        # 0 lfo speed freezes position
        if self.read_lfo is not None:
            if read_lfo_speed <= 0:
                read_lfo_speed = 0
            self.read_lfo_speed = read_lfo_speed
            self.read_lfo_length = len(self.read_lfo)

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
 
    def play(self, length=10):
        cdef int framelength = <int>(self.samplerate * length)
        cdef int input_length = len(self.buf)

        cdef int write_pos = 0              # frame position in output buffer for writing
        cdef int start = 0                  # grain start position in input buffer (frames)
        cdef int end = 0                    # grain end position in input buffer (frames)
        cdef double panpos = 0.5

        ###########
        # Read LFO
        ###########
        cdef double read_frac_pos = 0       # 0-1 pos in read buffer
        cdef double read_frac_pos_next = 0  # next value of 0-1 pos in read buffer for interpolation
        cdef double read_frac = 0           # fractional index for interpolation
        cdef double read_phase = 0
        cdef double read_phase_inc = (1.0 / framelength) * self.read_lfo_length
        cdef int read_pos = 0               # frame position in input buffer for reading
        cdef int read_index = 0

        ############
        # Speed LFO
        ############
        cdef double speed_frac_pos = 0       # 0-1 pos in speed buffer
        cdef double speed_frac_pos_next = 0  # next value of 0-1 pos in speed buffer for interpolation
        cdef double speed_frac = 0           # fractional index for interpolation
        cdef double speed_phase = 0
        cdef double speed_phase_inc = (1.0 / framelength) * self.speed_lfo_length
        cdef int speed_index = 0

        ##############
        # Density LFO
        ##############
        cdef double density_frac_pos = 0       # 0-1 pos in density buffer
        cdef double density_frac_pos_next = 0  # next value of 0-1 pos in density buffer for interpolation
        cdef double density_frac = 0           # fractional index for interpolation
        cdef double density_phase = 0
        cdef double density_phase_inc = (1.0 / framelength) * self.density_lfo_length
        cdef int density_pos = 0              # frame position in output buffer for writing
        cdef int density_index = 0

        ###################
        # Grain Length LFO
        ###################
        cdef double grainlength_frac_pos = 0       # 0-1 pos in grainlength buffer
        cdef double grainlength_frac_pos_next = 0  # next value of 0-1 pos in grainlength buffer for interpolation
        cdef double grainlength_frac = 0           # fractional index for interpolation
        cdef double grainlength_phase = 0
        cdef double grainlength_phase_inc = (1.0 / framelength) * self.grainlength_lfo_length
        cdef int grainlength_pos = 0              # frame position in output buffer for writing
        cdef int grainlength_index = 0
        cdef int grainlength = <int>(<double>self.samplerate * self.grainlength * 0.001)
        cdef int adjusted_grainlength = 0

        cdef int max_read_pos = input_length - grainlength

        cdef SoundBuffer out = SoundBuffer(length=length, channels=self.channels, samplerate=self.samplerate)

        cdef double maxspeed = <double>(input_length/4) / <double>min(self.grainlength, self.minlength)
        if self.maxspeed > maxspeed:
            # re-cap maxspeed based on grainlength
            self.maxspeed = maxspeed

        if self.speed > maxspeed:
            # re-cap maxspeed based on grainlength
            self.speed = maxspeed

        cdef int count = 0
        while write_pos < framelength - grainlength:
            # Grain length LFO
            if self.grainlength_lfo is not None:
                grainlength_phase = (<double>write_pos / framelength) * <double>self.grainlength_lfo_length * self.grainlength_lfo_speed
                grainlength_index = <int>grainlength_phase
                grainlength_frac = grainlength_phase - grainlength_index
                grainlength_frac_pos = self.grainlength_lfo[grainlength_index % self.grainlength_lfo_length]
                grainlength_frac_pos_next = self.grainlength_lfo[(grainlength_index+1) % self.grainlength_lfo_length]
                grainlength_frac_pos = (1.0 - grainlength_frac) * grainlength_frac_pos + grainlength_frac * grainlength_frac_pos_next
                grainlength_phase += grainlength_phase_inc

                if self.jitter > 0:
                    grainlength_frac_pos = grainlength_frac_pos * ((rand()/<double>RAND_MAX) * self.jitter + 1)

                grainlength = <int>(((grainlength_frac_pos * (self.maxlength - self.minlength)) + self.minlength) * (self.samplerate / 1000.0))
                max_read_pos = input_length - grainlength

            # Read LFO
            if self.read_lfo is None:
                if self.freeze < 0:
                    self.freeze = 0
                read_frac_pos = (self.samplerate * self.freeze) / <double>max_read_pos
                if read_frac_pos > 1:
                    read_frac_pos = 1
            else:
                read_phase = (<double>write_pos / (framelength - grainlength)) * <double>self.read_lfo_length * self.read_lfo_speed
                read_index = <int>read_phase
                read_frac = read_phase - read_index
                read_frac_pos = self.read_lfo[read_index % self.read_lfo_length]
                read_frac_pos_next = self.read_lfo[(read_index+1) % self.read_lfo_length]
                read_frac_pos = (1.0 - read_frac) * read_frac_pos + read_frac * read_frac_pos_next
                read_phase += read_phase_inc

            # Speed LFO
            if self.speed_lfo is not None:
                speed_phase = (<double>write_pos / (framelength - grainlength)) * <double>self.speed_lfo_length
                speed_index = <int>speed_phase
                speed_frac = speed_phase - speed_index
                speed_frac_pos = self.speed_lfo[speed_index % self.speed_lfo_length]
                speed_frac_pos_next = self.speed_lfo[(speed_index+1) % self.speed_lfo_length]
                speed_frac_pos = (1.0 - speed_frac) * speed_frac_pos + speed_frac * speed_frac_pos_next
                speed_phase += speed_phase_inc

            # Density LFO
            if self.density_lfo is not None:
                density_phase = (<double>write_pos / (framelength - grainlength)) * <double>self.density_lfo_length * self.density_lfo_speed
                density_index = <int>density_phase
                density_frac = density_phase - density_index
                density_frac_pos = self.density_lfo[density_index % self.density_lfo_length]
                density_frac_pos_next = self.density_lfo[(density_index+1) % self.density_lfo_length]
                density_frac_pos = (1.0 - density_frac) * density_frac_pos + density_frac * density_frac_pos_next
                density = (density_frac_pos * (self.maxdensity - self.mindensity)) + self.mindensity
                density_phase += density_phase_inc

            else:
                density = self.density

            if density < MIN_DENSITY:
                density = MIN_DENSITY

            if self.speed_lfo is not None:
                self.speed = speed_frac_pos * (self.maxspeed - self.minspeed) + self.minspeed

            #if self.mask is not None and self.mask[count] == 0:
            #    pass

            if self.speed != 1:
                adjusted_grainlength = <int>(grainlength * self.speed)
                if adjusted_grainlength > MIN_GRAIN_FRAMELENGTH:

                    start = <int>(read_frac_pos * (input_length-adjusted_grainlength))
                    grain = self.buf[start:start+adjusted_grainlength] 
                    grain = grain.speed(self.speed)
                else:
                    write_pos += MIN_GRAIN_FRAMELENGTH
                    continue
            else:
                start = <int>(read_frac_pos * max_read_pos)
                grain = self.buf[start:start+grainlength] 


            grain = grain * self.win

            if self.spread > 0:
                panpos = (rand()/<double>RAND_MAX) * self.spread + (0.5 - (self.spread * 0.5))
                grain = grain.pan(panpos)

            if write_pos + len(grain) < len(out):
                out._dub(grain * self.amp, write_pos)

            self.grains_per_sec = <double>self.samplerate / (<double>len(grain) / 2.0)
            self.grains_per_sec *= density

            write_pos += <int>(<double>self.samplerate / self.grains_per_sec)
            count += 1

        return out



