# cython: language_level=3

from pippi.wavetables cimport HANN, PHASOR, to_window, to_wavetable
from pippi cimport interpolation
from pippi.soundpipe cimport *
from pippi.soundbuffer cimport SoundBuffer
from libc.stdlib cimport rand, RAND_MAX, malloc, free
import numpy as np
from cpython cimport array
import array

DEF MIN_DENSITY = 0.000001
DEF MIN_ED = 0.000001
DEF MIN_GRAIN_FRAMELENGTH = 441
DEF DEFAULT_WTSIZE = 4096
DEF DEFAULT_GRAINLENGTH = 60
DEF DEFAULT_MAXGRAINLENGTH = 80
DEF DEFAULT_ED = 1
DEF DEFAULT_MAXED = 2

cdef class Cloud:
    def __cinit__(self, 
            SoundBuffer snd, 
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
        ):

        self.snd = memoryview2ftbls(snd.frames)
        self.framelength = <unsigned int>len(snd)
        self.length = <double>(self.framelength / <double>snd.samplerate)
        self.channels = <unsigned int>snd.channels
        self.samplerate = <unsigned int>snd.samplerate

        self.wtsize = wtsize

        if window is None:
            window = HANN
        self.window = to_window(window)

        if position is None:
            position = PHASOR
        self.position = to_window(position)

        self.amp = to_wavetable(amp)
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
        cdef unsigned int c = 0
        for c in range(self.channels):
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
        cdef sp_data* sp
        cdef sp_mincer* mincer
        cdef sp_ftbl* tbl

        sp_create(&sp)

        if self.has_mask:
            masklength = <unsigned int>len(self.mask)

        #while write_pos < outframelength - grainlength: 
        while pos <= 1: 
            if self.has_mask and self.mask[<int>(count % masklength)] == 0:
                write_pos += <unsigned int>(interpolation._linear_point(self.grid, pos) * self.samplerate)
                pos = <double>write_pos / <double>outframelength
                count += 1
                continue

            grainlength = <unsigned int>(interpolation._linear_point(self.grainlength, pos) * self.samplerate)
            read_pos = <unsigned int>(interpolation._linear_point(self.position, pos) * self.framelength)

            spread = interpolation._linear_point(self.spread, pos)
            panpos = (rand()/<double>RAND_MAX) * spread + (0.5 - (spread * 0.5))
            pans = [panpos, 1-panpos]

            for c in range(self.channels):
                sp_ftbl_bind(sp, &tbl, self.snd[c], self.framelength)
                sp_mincer_create(&mincer)
                sp_mincer_init(sp, mincer, tbl, self.wtsize)

                panpos = pans[c % 2]

                for i in range(grainlength):
                    grainpos = <double>i / grainlength
                    if read_pos+i < self.framelength and write_pos+i < outframelength:
                        mincer.time = <float>(<float>(read_pos+i)/<float>self.samplerate)
                        mincer.pitch = <float>interpolation._linear_point(self.speed, pos)
                        mincer.amp = <float>interpolation._linear_point(self.amp, pos)

                        sp_mincer_compute(sp, mincer, NULL, &fsample)
                        sample = <double>fsample * interpolation._linear_point(self.window, grainpos)
                        sample *= panpos
                        out[i+write_pos,c] += sample

                sp_mincer_destroy(&mincer)
                sp_ftbl_destroy(&tbl)

            #print(write_pos, pos, count)
            write_pos += <unsigned int>(interpolation._linear_point(self.grid, pos) * self.samplerate)
            pos = <double>write_pos / <double>outframelength
            count += 1

        sp_destroy(&sp)

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)


