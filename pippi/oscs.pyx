import numbers
import numpy as np
from .soundbuffer cimport SoundBuffer
from . import wavetables
from . cimport wavetables
from . cimport interpolation

cimport cython
from cpython.array cimport array, clone

cdef inline int DEFAULT_SAMPLERATE = 44100
cdef inline int DEFAULT_CHANNELS = 2
cdef inline double MIN_PULSEWIDTH = 0.0001

cdef class Osc:
    """ 1d or 2d wavetable osc
    """
    cdef double freq
    cdef double amp

    cdef double[:] wavetable
    cdef list wavetables
    cdef double lfo_freq
    cdef object lfo

    cdef double[:] window
    cdef double pulsewidth

    cdef double[:] mod
    cdef double mod_range
    cdef double mod_freq

    cdef double phase
    cdef double win_phase
    cdef double mod_phase

    cdef int channels
    cdef int samplerate
    cdef int wtsize

    def __init__(
            self, 
            object wavetable=None, 
              list stack=None,
            double freq=440, 
            double amp=1, 
            double pulsewidth=1,
            double phase=0, 

            object window=None, 
            double win_phase=0, 

            object mod=None, 
            double mod_freq=0.2, 
            double mod_range=0, 
            double mod_phase=0, 

            object lfo=None, 
            double lfo_freq=0.2, 

            int wtsize=4096,
            int channels=DEFAULT_CHANNELS,
            int samplerate=DEFAULT_SAMPLERATE,
        ):

        self.freq = freq
        self.amp = amp
        self.phase = phase

        self.channels = channels
        self.samplerate = samplerate
        self.wtsize = wtsize

        self.pulsewidth = pulsewidth if pulsewidth >= MIN_PULSEWIDTH else MIN_PULSEWIDTH

        if isinstance(wavetable, int):
            # Create wavetable type if wavetable is an integer flag
            self.wavetable = wavetables._wavetable(wavetable, self.wtsize)

        elif isinstance(wavetable, wavetables.Wavetable):
            self.wavetable = wavetable.data

        elif isinstance(wavetable, SoundBuffer):
            if wavetable.channels > 1:
                wavetable = wavetable.remix(1)
            self.wavetable = wavetable.frames

        elif wavetable is not None:
            self.wavetable = array('d', wavetable)

        if stack is not None:
            self.wavetables = []
            for i, wt in enumerate(stack):
                if isinstance(wt, int):
                    self.wavetables += [ wavetables._wavetable(wt, self.wtsize) ]
                elif isinstance(wt, wavetables.Wavetable):
                    self.wavetables += [ interpolation._linear(wt.data, self.wtsize) ]
                elif isinstance(wt, SoundBuffer):
                    if wt.channels > 1:
                        wt = wt.remix(1)
                    self.wavetables += [ interpolation._linear(wt.frames, self.wtsize) ]
                else:
                    self.wavetables += [ interpolation._linear(array('d', wt), self.wtsize) ]

        self.win_phase = win_phase
        if isinstance(window, int):
            self.window = wavetables._window(window, self.wtsize)
        elif isinstance(window, wavetables.Wavetable):
            self.window = interpolation._linear(window.data, self.wtsize)
        elif isinstance(window, SoundBuffer):
            if window.channels > 1:
                window = window.remix(1)
            self.window = interpolation._linear(window.frames, self.wtsize)
        elif window is not None:
            self.window = interpolation._linear(array('d', window), self.wtsize)
        else:
            self.window = None

        self.mod_range = mod_range
        self.mod_phase = mod_phase
        self.mod_freq = mod_freq
        if isinstance(mod, int):
            self.mod = wavetables._window(mod, self.wtsize)
        elif isinstance(mod, wavetables.Wavetable):
            self.mod = interpolation._linear(mod.data, self.wtsize)
        elif isinstance(mod, SoundBuffer):
            if mod.channels > 1:
                mod = mod.remix(1)
            self.mod = interpolation._linear(mod.frames, self.wtsize)
        elif isinstance(mod, list):
            self.mod = interpolation._linear(array('d', mod), self.wtsize)
        else:
            self.mod = None

        self.lfo_freq = lfo_freq
        if isinstance(lfo, int):
            self.lfo = wavetables._window(lfo, self.wtsize)
        elif isinstance(lfo, wavetables.Wavetable):
            self.lfo = interpolation._linear(lfo.data, self.wtsize)
        elif isinstance(lfo, SoundBuffer):
            if lfo.channels > 1:
                lfo = lfo.remix(1)
            self.lfo = interpolation._linear(lfo.frames, self.wtsize)
        elif isinstance(lfo, list):
            self.lfo = interpolation._linear(array('d', lfo), self.wtsize)
        else:
            self.lfo = None

        if stack is not None and self.lfo is None:
            self.lfo = wavetables._wavetable(wavetables.RSAW, self.wtsize)

    cpdef object play(self, 
             double length, 
             double freq=-1, 
             double amp=-1, 
             double pulsewidth=-1,
             double mod_freq=-1,
             double mod_range=-1,
        ):

        framelength = <int>(length * self.samplerate)

        if freq > 0:
            self.freq = freq

        if amp >= 0:
            self.amp = amp 

        if pulsewidth > 0:
            self.pulsewidth = pulsewidth 

        if mod_freq > 0:
            self.mod_freq = mod_freq 

        if mod_range >= 0:
            self.mod_range = mod_range 

        if self.wavetables is not None:
            return self._play2d(framelength)
        else:
            return self._play(framelength)

    cdef object _play(self, int length):
        cdef double[:,:] out = np.zeros((length, self.channels), dtype='d')

        cdef int i = 0
        cdef int wtindex = 0
        cdef int modindex = 0
        cdef int wtlength = len(self.wavetable)
        cdef int wtboundry = wtlength - 1
        cdef int modlength = 1 if self.mod is None else len(self.mod)
        cdef int modboundry = modlength - 1
        cdef double val, val_mod, nextval, nextval_mod, frac, frac_mod
        cdef int silence_length

        cdef double[:] wavetable = self.wavetable
        cdef double[:] window

        cdef unsigned int j = 0

        if self.window is not None:
            window = interpolation._linear(self.window, wtlength)
            for j in range(wtlength):
                wavetable[j] *= window[j]

        if self.pulsewidth < 1:
            silence_length = <int>((wtlength / self.pulsewidth) - wtlength)
            wavetable = np.concatenate((wavetable, np.zeros(silence_length, dtype='d')))

        cdef double isamplerate = 1.0 / self.samplerate

        for i in range(length):
            wtindex = <int>self.phase % wtlength
            modindex = <int>self.mod_phase % modlength

            val = wavetable[wtindex]
            val_mod = 1

            if wtindex < wtboundry:
                nextval = wavetable[wtindex + 1]
            else:
                nextval = wavetable[0]

            if self.mod is not None:
                if modindex < modboundry:
                    nextval_mod = self.mod[modindex + 1]
                else:
                    nextval_mod = self.mod[0]

                frac_mod = self.mod_phase - <int>self.mod_phase
                val_mod = self.mod[modindex]
                val_mod = (1.0 - frac_mod) * val_mod + frac_mod * nextval_mod
                val_mod = 1.0 + (val_mod * self.mod_range)
                self.mod_phase += self.mod_freq * modlength * isamplerate

            frac = self.phase - <int>self.phase
            val = ((1.0 - frac) * val + frac * nextval) * self.amp
            self.phase += self.freq * val_mod * wtlength * isamplerate

            for channel in range(self.channels):
                out[i][channel] = val

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

    cdef object _play2d(self, int length):
        cdef double[:,:] out = np.zeros((length, self.channels), dtype='d')
        cdef double[:,:] stack = np.column_stack(self.wavetables)
        cdef int stack_depth = len(self.wavetables)

        cdef double wt_lfo_phase_inc = len(self.lfo) * (1.0 / length)
        cdef double phase_inc = self.freq * self.wtsize * (1.0 / self.samplerate)
        cdef double wt_phase_inc = self.freq * self.wtsize * (1.0 / self.samplerate)

        cdef double wt_lfo_phase, wt_lfo_frac, wt_lfo_y0, wt_lfo_y1, wt_lfo_pos
        cdef double stack_frac, stack_phase
        cdef double phase, frac, y0, y1, val

        cdef int i, wt_lfo_x, stack_x, channel

        for i in range(length):
            wt_lfo_phase = i * wt_lfo_phase_inc
            wt_lfo_x = <int>wt_lfo_phase
            wt_lfo_frac = wt_lfo_phase - wt_lfo_x
            wt_lfo_y0 = self.lfo[wt_lfo_x % len(self.lfo)]
            wt_lfo_y1 = self.lfo[(wt_lfo_x + 1) % len(self.lfo)]

            wt_lfo_pos = (1.0 - wt_lfo_frac) * wt_lfo_y0 + (wt_lfo_frac * wt_lfo_y1)

            stack_phase = wt_lfo_pos * stack_depth
            stack_x = <int>stack_phase
            wt_phase = i * wt_phase_inc
            wt_x = <int>wt_phase
            stack_frac = stack_phase - stack_x

            y0 = self.wavetables[stack_x % stack_depth][wt_x % self.wtsize]
            y1 = self.wavetables[(stack_x + 1) % stack_depth][wt_x % self.wtsize]
            val = (1.0 - stack_frac) * y0 + (stack_frac * y1)

            for channel in range(self.channels):
                out[i][channel] = val * self.amp

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)


cdef class Fold:
    """ Folding wavetable oscilator
    """
    cdef public double[:] wavetable
    cdef public double[:] factors
    cdef public double factFreq
    cdef public double freq
    cdef public double amp
    cdef int channels
    cdef int samplerate
    cdef int wtsize


    def __init__(
            self, 
            double[:] wavetable=None, 
            double[:] factors=None, 
            double freq=440, 
            double factFreq=1,
            double amp=1, 
            int wtsize=4096,
            int channels=DEFAULT_CHANNELS,
            int samplerate=DEFAULT_SAMPLERATE,
        ):

        self.freq = freq
        self.factFreq = factFreq
        self.amp = amp

        self.channels = channels
        self.samplerate = samplerate
        self.wtsize = wtsize

        self.wavetable = wavetable
        self.factors = factors

    def play(self, double length):
        return self._play(length)

    cdef SoundBuffer _play(self, double length):
        cdef int framelength = <int>(length * self.samplerate)
        cdef int i
        cdef int channel = 0

        cdef double valWaveform, valNextWaveform, freq, factFreq
        cdef int cIndexWaveform = 0
        cdef double fracWaveform = 0

        cdef double valFactors, valNextFactors
        cdef int cIndexFactors = 0
        cdef double fracFactors = 0

        cdef double indexWaveform = 0
        cdef double indexFactors = 0

        cdef int lenWaveform = len(self.wavetable)
        cdef int lenFactors = len(self.factors)

        cdef double fold_out = 0
        cdef double last_value = 0 
        cdef int pos_thresh = 1 
        cdef int neg_thresh = -1 
        cdef int state = 1
        cdef double difference = 0

        cdef double[:,:] out = np.zeros((framelength, self.channels), dtype='d')

        for i in range(framelength):
            # Interp waveform wavetable
            cIndexWaveform = <int>indexWaveform % (lenWaveform - 1)
            valWaveform = self.wavetable[cIndexWaveform]
            valNextWaveform = self.wavetable[cIndexWaveform + 1]
            fracWaveform = indexWaveform - <int>indexWaveform
            valWaveform = (1.0 - fracWaveform) * valWaveform + fracWaveform * valNextWaveform

            # Interp factors wavetable
            cIndexFactors = <int>indexFactors % (lenFactors - 1)
            valFactors = self.factors[cIndexFactors]
            valNextFactors = self.factors[cIndexFactors + 1]
            fracFactors = indexFactors - <int>indexFactors
            valFactors = (1.0 - fracFactors) * valFactors + fracFactors * valNextFactors
            valWaveform *= valFactors

            # Do the folding 
            # the magic is ripped from guest's posts on muffwiggler: 
            # http://www.muffwiggler.com/forum/viewtopic.php?p=1586526#1586526
            difference = valWaveform - last_value
            last_value = valWaveform
            if state == 1:
                fold_out -= difference
            else:
                fold_out += difference

            if fold_out >= pos_thresh:
                state ^= 1
                fold_out = pos_thresh - (fold_out - pos_thresh)
            elif fold_out <= neg_thresh:
                state ^= 1
                fold_out = neg_thresh - (fold_out - neg_thresh)

            for channel in range(self.channels):
                out[i][channel] = self.amp * fold_out

            indexWaveform += self.freq * lenWaveform * (1.0 / self.samplerate)
            indexFactors += self.factFreq * lenFactors * (1.0 / self.samplerate)

        return SoundBuffer(out, channels=self.channels, samplerate=self.samplerate)

