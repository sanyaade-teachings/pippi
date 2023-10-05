cimport cython
cimport numpy as np
import numpy as np

from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport Wavetable, to_window

np.import_array()


@cython.boundscheck(False)
@cython.wraparound(False)
cdef inline double _linear_pos(double[:] data, double phase) nogil:
    cdef int dlength = <int>len(data)
    phase *= <double>(dlength-1)

    if dlength == 1:
        return data[0]

    elif dlength < 1:
        return 0

    cdef double frac = phase - <long>phase
    cdef long i = <long>phase
    cdef double a, b

    if i >= dlength-1:
        return 0

    a = data[i]
    b = data[i+1]

    return (1.0 - frac) * a + (frac * b)

cdef lpbuffer_t * to_lpbuffer_window_from_wavetable(Wavetable window):
    cdef lpbuffer_t * win
    cdef int i, length

    length = len(window.data)
    win = LPBuffer.create(length, 1, 48000)

    for i in range(length):
        win.data[i] = <lpfloat_t>window.data[i]

    return win

cdef lpbuffer_t * to_lpbuffer_window_from_soundbuffer(SoundBuffer window):
    cdef lpbuffer_t * win
    cdef int i, length

    length = len(window.frames)
    win = LPBuffer.create(4096, 1, 48000)

    for i in range(length):
        win.data[i] = <lpfloat_t>window.frames[i,0]

    return win

cdef int to_window_flag(str window_name=None):
    if window_name == 'sine':
        return WIN_SINE

    if window_name == 'sinein':
        return WIN_SINEIN

    if window_name == 'sineout':
        return WIN_SINEOUT

    if window_name == 'cos':
        return WIN_COS

    if window_name == 'tri':
        return WIN_TRI

    if window_name == 'phasor':
        return WIN_PHASOR

    if window_name == 'hann':
        return WIN_HANN

    if window_name == 'hannin':
        return WIN_HANNIN

    if window_name == 'hannout':
        return WIN_HANNOUT

    if window_name == 'rnd':
        return WIN_RND

    if window_name == 'saw':
        return WIN_SAW

    if window_name == 'rsaw':
        return WIN_RSAW

    return WIN_HANN

cdef class Cloud2:
    def __cinit__(self, 
            SoundBuffer snd, 
            object window=None, 
            object position=None,
            object amp=1.0,
            object speed=1.0, 
            object spread=0.0, 
            object jitter=0.0, 
            object grainlength=0.2, 
            object grid=None,
            object mask=None,
            object phase=None,
            int numgrains=2,
            unsigned int wtsize=4096,
        ):
        """ TODO:
            [ ] position
            [ ] amp 
            [ ] speed
            [ ] spread
            [ ] jitter
            [ ] mask
            [ ] numgrains modulation
        """

        cdef size_t i, c, sndlength
        cdef lpbuffer_t * win
        cdef int window_type

        self.channels = snd.channels
        self.samplerate = snd.samplerate
        self.grainlength = to_window(grainlength)

        if isinstance(window, SoundBuffer):
            window_type = WIN_USER
            win = to_lpbuffer_window_from_soundbuffer(window)

        elif isinstance(window, Wavetable):
            window_type = WIN_USER
            win = to_lpbuffer_window_from_wavetable(window)

        else:
            window_type = to_window_flag(window)
            win = NULL

        if phase is None:
            phase = [0,1]
        self.phase = to_window(phase)

        if grid is None:
            self.grid = np.multiply(self.grainlength, 0.3)
        else:
            self.grid = to_window(grid)

        sndlength = <size_t>len(snd.frames)
        srcbuf = LPBuffer.create(sndlength, self.channels, self.samplerate)

        for i in range(sndlength):
            for c in range(self.channels):
                srcbuf.data[i * self.channels + c] = snd.frames[i, c]

        self.formation = LPFormation.create(
                window_type,
                numgrains, 
                0, 
                sndlength, 
                self.channels, 
                self.samplerate,
                win
        )

        LPRingBuffer.write(self.formation.rb, srcbuf)
        LPBuffer.destroy(srcbuf)

    def __dealloc__(self):
        if self.formation != NULL:
            LPFormation.destroy(self.formation)

    def play(self, double length):
        cdef size_t i, c, framelength
        cdef SoundBuffer out
        cdef double phase=0

        framelength = <size_t>(length * self.samplerate)
        out = SoundBuffer(length=length, channels=self.channels, samplerate=self.samplerate)

        for i in range(framelength):
            phase = _linear_pos(self.phase, <double>i / framelength)
            self.formation.grainlength = <size_t>(_linear_pos(self.grainlength, phase) * self.samplerate)
            self.formation.graininterval = <size_t>(_linear_pos(self.grid, phase) * self.samplerate)

            LPFormation.process(self.formation)
            for c in range(self.channels):
                out.frames[i, c] = self.formation.current_frame.data[c]

        return out

