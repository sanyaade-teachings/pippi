import numbers
import numpy as np
from .soundbuffer import SoundBuffer
from . cimport wavetables
from . cimport interpolation

cimport cython
from cpython.array cimport array, clone

ctypedef fused duration:
    cython.int
    cython.double

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

        if isinstance(wavetable, list) and not isinstance(wavetable[0], numbers.Real):
            self.wavetable = None
            self.wavetables = []
            for i, wt in enumerate(wavetable):
                if isinstance(wt, str):
                    self.wavetables += [ wavetables._wavetable(wt, self.wtsize) ]
                else:
                    if len(wt) != self.wtsize:
                        wt = interpolation._linear(wt, self.wtsize)
                    self.wavetables += [ wt ]
        else:
            self.wavetables = None
            if wavetable is None:
                wavetable = 'sine'

            if isinstance(wavetable, str):
                self.wavetable = wavetables._wavetable(wavetable, self.wtsize)
            else:
                self.wavetable = np.array(wavetable, dtype='d')

        self.win_phase = win_phase
        if isinstance(window, str):
            self.window = wavetables._window(window, self.wtsize)
        elif window is not None:
            self.window = np.array(window, dtype='d')
        else:
            self.window = None

        self.mod_range = mod_range
        self.mod_phase = mod_phase
        self.mod_freq = mod_freq
        if isinstance(mod, str):
            self.mod = wavetables._window(mod, self.wtsize)
        elif mod is not None:
            self.mod = np.array(mod, dtype='d')
        else:
            self.mod = None

        self.lfo_freq = lfo_freq
        if isinstance(lfo, str):
            self.lfo = wavetables._window(lfo, self.wtsize)
        elif lfo is not None:
            self.lfo = np.array(lfo, dtype='d')
        else:
            self.lfo = None

    cpdef object play(self, 
             duration length, 
             double freq=-1, 
             double amp=-1, 
             double pulsewidth=-1,
             double mod_freq=-1,
             double mod_range=-1,
        ):

        if duration is double:
            length = <int>(length * self.samplerate)

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
            return self._play2d(<int>length)
        else:
            return self._play(<int>length)

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

        if self.window is not None:
            window = interpolation._linear(self.window, wtlength)
            wavetable = np.multiply(wavetable, window)

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
    cdef public object wavetable
    cdef public object factors
    cdef public double factFreq
    cdef public double freq
    cdef public double amp

    def __init__(
            self, 
            object wavetable=None, 
            object factors=None, 
            double freq=440, 
            double factFreq=1,
            double amp=1, 
        ):

        self.freq = freq
        self.factFreq = factFreq
        self.amp = amp

        if isinstance(wavetable, str):
            self.wavetable = wavetables._wavetable(wavetable, 4096)
        else:
            self.wavetable = wavetable

        if isinstance(factors, str):
            self.factors = wavetables._window(factors, 4096)
        else:
            self.factors = factors


    def play(self, int length, int channels=2, int samplerate=44100):
        return self._play(length, channels, samplerate)

    cdef object _play(self, int length, int channels=2, int samplerate=44100):
        cdef int i

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

        cdef double[:,:] out = np.zeros((length, channels), dtype='d')

        for i in range(length):
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

            for channel in range(channels):
                out[i][channel] = self.amp * fold_out

            indexWaveform += self.freq * lenWaveform * (1.0 / samplerate)
            indexFactors += self.factFreq * lenFactors * (1.0 / samplerate)

        return SoundBuffer(out, channels=channels, samplerate=samplerate)

"""
cdef class Scrub:
    # Granular scrubbing thingy 
    #
    cdef public SoundBuffer snd
    cdef public object wavetable
    cdef public object factors
    cdef public double factFreq
    cdef public double freq
    cdef public double amp

    def __init__(
            self, 
            SoundBuffer snd=None, 
            object scrub=None, 
            object window=None, 
            double freq=440, 
            double factFreq=1,
            double amp=1, 
        ):

        self.freq = freq
        self.factFreq = factFreq
        self.amp = amp

        if isinstance(wavetable, str):
            self.wavetable = wavetables._wavetable(wavetable, 4096)
        else:
            self.wavetable = wavetable

        if isinstance(factors, str):
            self.factors = wavetables._window(factors, 4096)
        else:
            self.factors = factors


    def play(self, int length, int channels=2, int samplerate=44100):
        return self._play(length, channels, samplerate)

    cdef object _play(self, int length, int channels=2, int samplerate=44100):
        cdef int cycle_length, input_length, cycle_start
        cdef int window_type = 0 # HANN
        cdef int scrub_type = 1 # SAW
        cdef double window_period = M_PI * 2.0
        cdef double scrub_period = 1.0

        cdef int size = 2
        cdef int channels = 2
        cdef int chunk = size + channels

        cdef double playhead, frequency
        cdef int i, f

        # Calculate the length of one cycle at the given frequency. 
        # This is a lossy calculation and introduces slight tuning errors
        # that become more extreme as the frequency increases. Deal with it.
        cycle_length = <int>(samplingrate / frequency) * chunk

        # Calculate the number of cycles needed to generate for the 
        # desired output length.
        cdef int num_cycles = length / cycle_length

        # Prefer overflows of length to underflows, so check for 
        # a reminder when calculating and tack an extra cycle up in there.
        if length % cycle_length > 0:
            num_cycles += 1

        # Generate a scrub position trajectory. The scrub position is 
        # just the position of the playhead at the start of each cycle.
        # 
        # The length of this array is equal to the number of cycles calculated 
        # earlier. 
        # 
        # Positions will range from 0 to the length of the input minus the length 
        # of a single cycle. 
        # 
        # These lengths should both be measured in frames. So: length / chunk.
        cdef int scrub_positions[num_cycles]
        cdef int max_position = (input_length / chunk) - (cycle_length / chunk)

        cdef double pos_curve[num_cycles]
        wavetable(scrub_type, pos_curve, num_cycles, 1.0, 0.0, 0.0, scrub_period)
        pos_curve = wavetables._wavetable(wavetable, num_cycles)

        for i in range(num_cycles):
            playhead = pos_curve[i]

            # Store the actual position in the buffer, not the frame count.
            scrub_positions[i] = <int>(playhead * max_position) * chunk

        # Generate each cycle of the stream by looping through the scrub array
        # and writing into the output buffer. For each cycle, read from the 
        # current scrub position forward cycle_length frames, and apply an 
        # amplitude envelope to each cycle as the buffer is filled.
        cdef double[:,:] out = np.zeros((length, channels))

        cdef double cycle[cycle_length / chunk]
        wavetable(window_type, cycle, cycle_length / chunk, 1.0, 0.0, 0.0, window_period);

        for i in range(num_cycles):
            cycle_start = scrub_positions[i]

            for f in range(cycle_length):
                for channel in range(channels):
                    out[i * cycle_length + f] = (cycle_start + f) * cycle[f / chunk]

        return out
"""

