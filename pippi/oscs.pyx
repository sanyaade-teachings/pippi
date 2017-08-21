import numpy as np
from .soundbuffer import SoundBuffer
from . import wavetables
from . import interpolation

from cpython.array cimport array, clone

cdef inline double MIN_PULSEWIDTH = 0.0001

cdef class Osc:
    """ Wavetable-based oscilator with some extras
    """
    cdef public double freq
    cdef public int offset
    cdef public double amp
    cdef public object wavetable
    cdef public object window
    cdef public object mod
    cdef public double mod_range
    cdef public double mod_freq
    cdef public double pulsewidth
    cdef double phase
    cdef double win_phase
    cdef double mod_phase

    def __init__(
            self, 
            double freq=440, 
            int offset=0, 
            double amp=1, 
            object wavetable=None, 
            object window=None, 
            object mod=None, 
            double mod_range=0.02, 
            double mod_freq=0.1, 
            double phase=0, 
            double pulsewidth=1
        ):

        self.freq = freq
        self.offset = offset
        self.amp = amp
        self.phase = phase
        self.win_phase = phase
        self.mod_phase = phase
        self.mod_range = mod_range 
        self.mod_freq = mod_freq
        self.pulsewidth = pulsewidth if pulsewidth >= MIN_PULSEWIDTH else MIN_PULSEWIDTH

        if wavetable is None:
            wavetable = 'sine'

        if isinstance(wavetable, str):
            self.wavetable = wavetables.wavetable(wavetable, 1024)
        else:
            self.wavetable = wavetable

        if isinstance(window, str):
            self.window = wavetables.window(window, 1024)
        else:
            self.window = window

        if isinstance(mod, str):
            self.mod = wavetables.window(mod, 1024)
        else:
            self.mod = mod

    def play(self, int length, int channels=2, int samplerate=44100):
        return self._play(length, channels, samplerate)

    cdef object _play(self, int length, int channels=2, int samplerate=44100):
        cdef double[:, :] out = np.zeros((length, channels))

        cdef int i = 0
        cdef int wtindex = 0
        cdef int modindex = 0
        cdef int wtlength = len(self.wavetable)
        cdef int modlength = 1 if self.mod is None else len(self.mod)
        cdef double val, val_mod, nextval, nextval_mod, frac, frac_mod

        wavetable = self.wavetable
        if self.window is not None:
            window = interpolation.linear(self.window, wtlength)
            wavetable = np.multiply(wavetable, window)

        if self.pulsewidth < 1:
            silence_length = int((wtlength / self.pulsewidth) - wtlength)
            wavetable = np.concatenate((wavetable, np.zeros(silence_length)))

        for i in range(length):
            wtindex = int(self.phase) % wtlength
            modindex = int(self.mod_phase) % modlength

            val = wavetable[wtindex]
            val_mod = 1

            try:
                nextval = wavetable[wtindex + 1]
            except IndexError:
                nextval = wavetable[0]

            if self.mod is not None:
                try:
                    nextval_mod = self.mod[modindex + 1]
                except IndexError:
                    nextval_mod = self.mod[0]

                frac_mod = self.mod_phase - int(self.mod_phase)
                val_mod = self.mod[modindex]
                val_mod = (1.0 - frac_mod) * val_mod + frac_mod * nextval_mod
                val_mod = 1.0 + (val_mod * self.mod_range)
                self.mod_phase += self.mod_freq * modlength * (1.0 / samplerate)

            frac = self.phase - int(self.phase)
            val = (1.0 - frac) * val + frac * nextval
            self.phase += self.freq * val_mod * wtlength * (1.0 / samplerate)

            for channel in range(channels):
                out[i][channel] = val * self.amp

        return SoundBuffer(out, channels=channels, samplerate=samplerate)

"""
static char pippic_fold_docstring[] = "Wave folding synthesis.";
static PyObject * pippic_fold(PyObject *self, PyObject *args) {
    PyObject *output;

    PyObject *waveformIn;
    PyObject *waveform;

    PyObject *factorsIn;
    PyObject *factors;

    signed char *data;

    int i;
    int size = getsize();
    int channels = 2;
    int chunk = size + channels;
    int length;

    double valWaveform, valNextWaveform, freq, factFreq, amp;
    int lenWaveform = 0, cIndexWaveform = 0;
    double fracWaveform = 0;

    double valFactors, valNextFactors;
    int lenFactors = 0, cIndexFactors = 0;
    double fracFactors = 0;

    double indexWaveform = 0, indexFactors = 0;

    if(!PyArg_ParseTuple(args, "ddiOOd", &freq, &factFreq, &length, &waveformIn, &factorsIn, &amp)) {
        return NULL;
    }

    length *= chunk;

    waveform = PySequence_Fast(waveformIn, "Could not read waveform wavetable.");
    lenWaveform = PySequence_Size(waveformIn);

    double wfrm[lenWaveform];
    for(i=0; i < lenWaveform; i++) {
        wfrm[i] = PyFloat_AsDouble(PyList_GET_ITEM(waveform, i));
    }

    factors = PySequence_Fast(factorsIn, "Could not read factors wavetable.");
    lenFactors = PySequence_Size(factorsIn);

    double facts[lenFactors];
    for(i=0; i < lenFactors; i++) {
        facts[i] = PyFloat_AsDouble(PyList_GET_ITEM(factors, i));
    }

    int fold_out = 0; 
    int last_value = 0; 
    int pos_thresh = 1; 
    int neg_thresh = -1; 
    int state = 1; 

    output = PyString_FromStringAndSize(NULL, length);
    data = (signed char*)PyString_AsString(output);

    for(i=0; i < length; i += chunk) {
        // Interp waveform wavetable
        cIndexWaveform = (int)indexWaveform % (lenWaveform - 1);
        valWaveform = wfrm[cIndexWaveform];
        valNextWaveform = wfrm[cIndexWaveform + 1];
        fracWaveform = indexWaveform - (int)indexWaveform;
        valWaveform = (1.0 - fracWaveform) * valWaveform + fracWaveform * valNextWaveform;

        // Interp factors wavetable
        cIndexFactors = (int)indexFactors % (lenFactors - 1);
        valFactors = facts[cIndexFactors];
        valNextFactors = facts[cIndexFactors + 1];
        fracFactors = indexFactors - (int)indexFactors;
        valFactors = (1.0 - fracFactors) * valFactors + fracFactors * valNextFactors;
        valWaveform *= valFactors;

        // Do the folding 
        // the magic is ripped from guest's posts on muffwiggler: 
        // http://www.muffwiggler.com/forum/viewtopic.php?p=1586526#1586526
        int difference = valWaveform - last_value; 
        last_value = valWaveform; 
        if(state == 1) { 
            fold_out -= difference; 
        } else { 
            fold_out += difference; 
        } 

        if (fold_out >= pos_thresh) { 
            state ^= 1; 
            fold_out = pos_thresh - (fold_out - pos_thresh); 
        } else if (fold_out <= neg_thresh) { 
            state ^= 1; 
            fold_out = neg_thresh - (fold_out - neg_thresh); 
        } 

        *BUFFER(data, i) = saturate(amp * fold_out * (MAXVAL - 1));
        *BUFFER(data, i + size) = saturate(amp * fold_out * (MAXVAL - 1));

        indexWaveform += freq * lenWaveform * (1.0 / 44100.0);
        indexFactors += factFreq * lenFactors * (1.0 / 44100.0);
    }

    Py_DECREF(waveform);
    Py_DECREF(factors);

    return output;
}





static char pippic_pine_docstring[] = "Just your average pinecone.";
static PyObject * pippic_pine(PyObject *self, PyObject *args) {
    PyObject *output;
    signed char *data, *input;

    int output_length, cycle_length, input_length, cycle_start;

    int window_type = HANN;
    int scrub_type = SAW;
    double window_period = M_PI * 2.0;
    double scrub_period = 1.0;


    int size = getsize();
    int channels = 2;
    int chunk = size + channels;

    double playhead, frequency;
    int i, f;

    if(!PyArg_ParseTuple(args, "s#id|idid", &input, &input_length, &output_length, &frequency, &window_type, &window_period, &scrub_type, &scrub_period)) {
        return NULL;
    }

    output_length *= chunk;

    /* Calculate the length of one cycle at the given frequency. 
     * This is a lossy calculation and introduces slight tuning errors
     * that become more extreme as the frequency increases. Deal with it.
     */
    cycle_length = (int)(44100.0 / frequency) * chunk;

    /* Calculate the number of cycles needed to generate for the 
     * desired output length.
     */
    int num_cycles = output_length / cycle_length;

    /* Prefer overflows of length to underflows, so check for 
     * a reminder when calculating and tack an extra cycle up in there.
     */
    if(output_length % cycle_length > 0) {
        num_cycles += 1;
    }

    /* Generate a scrub position trajectory. The scrub position is 
     * just the position of the playhead at the start of each cycle.
     *
     * The length of this array is equal to the number of cycles calculated 
     * earlier. 
     *
     * Positions will range from 0 to the length of the input minus the length 
     * of a single cycle. 
     *
     * These lengths should both be measured in frames. So: length / chunk.
     */
    int scrub_positions[num_cycles];
    int max_position = (input_length / chunk) - (cycle_length / chunk);

    double pos_curve[num_cycles];
    wavetable(scrub_type, pos_curve, num_cycles, 1.0, 0.0, 0.0, scrub_period);

    for(i=0; i < num_cycles; i++) {
        playhead = pos_curve[i];

        /* Store the actual position in the buffer, not the frame count. */
        scrub_positions[i] = (int)(playhead * max_position) * chunk;
    }

    /* Generate each cycle of the stream by looping through the scrub array
     * and writing into the output buffer. For each cycle, read from the 
     * current scrub position forward cycle_length frames, and apply an 
     * amplitude envelope to each cycle as the buffer is filled.
     */
    output = PyString_FromStringAndSize(NULL, output_length);
    data = (signed char*)PyString_AsString(output);

    double cycle[cycle_length / chunk];
    wavetable(window_type, cycle, cycle_length / chunk, 1.0, 0.0, 0.0, window_period);

    int left, right;

    for(i=0; i < num_cycles - 1; i++) {
        cycle_start = scrub_positions[i];

        for(f=0; f < cycle_length; f += chunk) {
            left = *BUFFER(input, cycle_start + f);
            *BUFFER(data, i * cycle_length + f) = saturate(left * cycle[f / chunk]);

            right = *BUFFER(input, cycle_start + f + size);
            *BUFFER(data, i * cycle_length + f + size) = saturate(right * cycle[f / chunk]);
        }
    }

    return output;
}

"""
