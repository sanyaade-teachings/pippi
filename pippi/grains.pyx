# cython: language_level=3

from pippi.wavetables cimport HANN, PHASOR, to_window, to_wavetable
from pippi cimport interpolation
from pippi.soundpipe cimport *
from pippi.soundbuffer cimport SoundBuffer
from libc.stdlib cimport rand, RAND_MAX, malloc, free
import numpy as np
from cpython cimport array
import array

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
            self.position = np.multiply(to_window(PHASOR), snd.dur)
        else:
            self.position = to_window(position)

        self.amp = to_window(amp)
        self.speed = to_window(speed)
        self.spread = to_window(spread)
        self.jitter = to_wavetable(jitter)
        self.grainlength = to_window(grainlength)

        if grid is None:
            self.grid = np.multiply(to_window(grainlength), 0.5)
        else:
            self.grid = to_window(grid)

        if lpf is None:
            self.has_lpf = False
        else:
            self.has_lpf = True
            self.lpf = to_window(lpf)

        if hpf is None:
            self.has_hpf = False
        else:
            self.has_hpf = True
            self.hpf = to_window(hpf)

        if bpf is None:
            self.has_bpf = False
        else:
            self.has_bpf = True
            self.bpf = to_window(bpf)

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
        cdef double read_pos = 0
        cdef unsigned int grainlength = 4410
        cdef unsigned int grainincrement = grainlength // 2
        cdef double pos = 0
        cdef double grainpos = 0
        cdef double panpos = 0
        cdef double sample = 0
        cdef double fsample = 0
        cdef double spread = 0
        cdef double inc = 0
        cdef double write_boundry = <double>(outframelength-1)
        cdef unsigned int masklength = 0
        cdef unsigned int count = 0
        cdef sp_data* sp
        cdef sp_mincer* mincer
        cdef sp_ftbl* tbl

        sp_create(&sp)

        if self.has_mask:
            masklength = <unsigned int>len(self.mask)

        while write_pos <= write_boundry: 
            if self.has_mask and self.mask[<int>(count % masklength)] == 0:
                write_pos += <unsigned int>(interpolation._linear_pos(self.grid, pos) * self.samplerate)
                pos = write_pos / write_boundry
                count += 1
                continue

            grainlength = <unsigned int>(interpolation._linear_pos(self.grainlength, pos) * self.samplerate)
            read_pos = interpolation._linear_pos(self.position, pos)

            spread = interpolation._linear_pos(self.spread, pos)
            panpos = (rand()/<double>RAND_MAX) * spread + (0.5 - (spread * 0.5))
            pans = [panpos, 1-panpos]

            for c in range(self.channels):
                sp_ftbl_bind(sp, &tbl, self.snd[c], self.framelength)
                sp_mincer_create(&mincer)
                sp_mincer_init(sp, mincer, tbl, self.wtsize)

                panpos = pans[c % 2]

                for i in range(grainlength):
                    grainpos = <double>i / grainlength
                    if write_pos+i < outframelength:
                        inc = <double>i / self.samplerate
                        mincer.time = <double>read_pos + inc
                        mincer.pitch = <double>interpolation._linear_pos(self.speed, pos)
                        mincer.amp = <double>interpolation._linear_pos(self.amp, pos)

                        sp_mincer_compute(sp, mincer, NULL, &fsample)
                        sample = <double>fsample * interpolation._linear_pos(self.window, grainpos)
                        sample *= panpos
                        out[i+write_pos,c] += sample

                sp_mincer_destroy(&mincer)
                sp_ftbl_destroy(&tbl)

            write_pos += <unsigned int>(interpolation._linear_point(self.grid, pos) * self.samplerate)

            write_jitter = <int>(interpolation._linear_pos(self.jitter, pos) * (rand()/<double>RAND_MAX) * self.samplerate)
            write_pos += max(0, write_jitter)

            pos = write_pos / write_boundry
            count += 1

        sp_destroy(&sp)

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)


