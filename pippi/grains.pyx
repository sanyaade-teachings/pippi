from .soundbuffer cimport SoundBuffer
from . cimport wavetables
from . cimport interpolation
from libc.stdlib cimport rand, RAND_MAX

DEF MIN_DENSITY = 0.000001
DEF MIN_GRAIN_FRAMELENGTH = 8
DEF DEFAULT_WTSIZE = 4096
DEF DEFAULT_GRAINLENGTH = 40
DEF DEFAULT_MAXGRAINLENGTH = 80
DEF DEFAULT_SPEED = 1
DEF DEFAULT_MAXSPEED = 2

cdef class GrainCloud:
    def __init__(self, 
            SoundBuffer buf, 

            int win=-1,
            double[:] win_wt=None,
            int win_length=DEFAULT_WTSIZE,

            int read_lfo=-1,
            double[:] read_lfo_wt=None,
            double read_lfo_speed=1,
            int read_lfo_length=DEFAULT_WTSIZE,

            int speed_lfo=-1,
            double[:] speed_lfo_wt=None,
            int speed_lfo_length=DEFAULT_WTSIZE,
            double speed=DEFAULT_SPEED,
            double minspeed=DEFAULT_SPEED, 
            double maxspeed=DEFAULT_MAXSPEED, 

            double density=1, 
            int density_lfo=-1,
            double[:] density_lfo_wt=None,
            int density_lfo_length=DEFAULT_WTSIZE,
            double density_lfo_speed=1,

            double grainlength=DEFAULT_GRAINLENGTH, 
            int grainlength_lfo=-1,
            double[:] grainlength_lfo_wt=None,
            int grainlength_lfo_length=DEFAULT_WTSIZE,
            double grainlength_lfo_speed=1,

            double minlength=DEFAULT_GRAINLENGTH,    # min grain length in ms
            double maxlength=DEFAULT_MAXGRAINLENGTH,    # max grain length in ms
            double spread=0,        # 0 = no panning, 1 = max random panning 
            double jitter=0,        # rhythm 0=regular, 1=totally random
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
            self.read_lfo = read_lfo_wt
        else:
            self.read_lfo = wavetables._window(wavetables.PHASOR, read_lfo_length)

        # 0 lfo speed freezes position
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

        self.speed = speed
        self.minspeed = minspeed
        self.maxspeed = maxspeed

        self.density = density
        if density_lfo > -1:
            self.density_lfo = wavetables._window(density_lfo, density_lfo_length)
        elif density_lfo_wt is not None:
            self.density_lfo = density_lfo_wt
        else:
            self.density_lfo = wavetables._window(wavetables.PHASOR, density_lfo_length)

        self.density_lfo_speed = density_lfo_speed
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

        cdef int max_read_pos = input_length - (grainlength*2)

        cdef SoundBuffer out = SoundBuffer(length=length, channels=self.channels, samplerate=self.samplerate)

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

            # Read LFO
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
            if self.density <= 0:
                density_phase = (<double>write_pos / (framelength - grainlength)) * <double>self.density_lfo_length * self.density_lfo_speed
                density_index = <int>density_phase
                density_frac = density_phase - density_index
                density_frac_pos = self.density_lfo[density_index % self.density_lfo_length]
                density_frac_pos_next = self.density_lfo[(density_index+1) % self.density_lfo_length]
                density_frac_pos = (1.0 - density_frac) * density_frac_pos + density_frac * density_frac_pos_next
                density = (density_frac_pos + 1) * self.density
                density_phase += density_phase_inc

            else:
                density = self.density

            if density < MIN_DENSITY:
                density = MIN_DENSITY

            start = <int>(read_frac_pos * (max_read_pos + grainlength))

            if self.speed_lfo is not None:
                self.speed = speed_frac_pos * (self.maxspeed - self.minspeed) + self.minspeed

            if self.speed != 1:
                grainlength = <int>(grainlength * (1.0/self.speed))
                if grainlength > MIN_GRAIN_FRAMELENGTH:
                    grain = self.buf[start:start+grainlength] 
                    grain = grain.speed(self.speed)
                else:
                    write_pos += MIN_GRAIN_FRAMELENGTH
                    continue
            else:
                grain = self.buf[start:start+grainlength] 

            grain = grain * self.win

            if self.spread > 0:
                panpos = (rand()/<double>RAND_MAX) * self.spread + (0.5 - (self.spread * 0.5))
                grain = grain.pan(panpos)

            if write_pos + len(grain) < len(out):
                out._dub(grain, write_pos)

            self.grains_per_sec = <double>self.samplerate / (len(grain) / 2.0)
            self.grains_per_sec *= density

            write_pos += <int>(<double>self.samplerate / self.grains_per_sec)
            count += 1

        return out



