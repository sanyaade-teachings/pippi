/* 
 * pippi.c - a simple dsp & synthesis api for the pippi python module 
 *
 * Much of this code is inspired by or based on the python standard 
 * library audioop module.
 *
 * */

#include "Python.h"
#include "math.h"

typedef int PyInt32;
typedef unsigned int Py_UInt32;

static PyObject *PippiError;

#define BUFFER(data, i) ((short *)(data + i))
#define MAXVAL 0x7fff
#define MINVAL -0x7fff

#define SINE 0
#define WT_SINE(position, amp, phase, offset, period) \
               (sin(position * period + phase) * amp + offset)

#define COS  1 
#define WT_COS(position, amp, phase, offset, period) \
               (cos(position * period + phase) * amp + offset)

#define HANN 2
#define WT_HANN(position, amp, phase, offset, period) \
               ((0.5 * (1.0 - cos(position * period + phase))) * amp + offset)

#define TRI  3
#define WT_TRI(position, amp, phase, offset, period) \
               ((1.0 - fabs((fmod(position * period + phase, 1.0) - 0.5) / 0.5)) * amp + offset)

#define SAW  4
#define WT_SAW  0 /* Sawtooth or line */

#define RSAW 5
#define WT_RSAW 0 /* Reversed sawtooth or phasor */

#define VARY 6
#define WT_VARY 0 /* Variable breakpoint envelope */

#define PULSE 7
#define SQUARE 8 
#define FLAT 9

static void wavetable(int waveform_type, double *buffer, int length, double amp, double phase, double offset, double period) {
    int i, last;
    double position, swap;
    int pulse_length = 3; /* TODO: makes sense to be able to specify a pulse width */

    switch(waveform_type) {
        case COS:
            for(i=0; i < length; i++) {
                position = (double)i / (double)(length);
                buffer[i] = WT_COS(position, amp, phase, offset, period);
            }
            break;

        case HANN:
            for(i=0; i < length; i++) {
                position = (double)i / (double)length;
                buffer[i] = WT_HANN(position, amp, phase, offset, period);
            }
            break;

        case TRI:
            for(i=0; i < length; i++) {
                position = (double)i / (double)length;
                buffer[i] = WT_TRI(position, amp, phase, offset, period);
            }
            break;

        case SAW:
            for(i=0; i < length; i++) {
                buffer[i] = (double)i / (double)(length - 1);
            }
            break;

        case RSAW:
            for(i=0; i < length; i++) {
                buffer[i] = (double)i / (double)(length - 1);
            }

            last = length - 1;
            for(i=0; i < length/2; i++) {
                swap = buffer[i];
                buffer[i] = buffer[last];
                buffer[last] = swap;
                last--;
            }
            break;

        case VARY:
            srand(time(NULL));

            for(i=0; i < length; i++) {
                position = (double)i / (double)length;
                position = WT_SINE(position, amp, phase, offset, period);
                position = (((double)(rand() % length) / (double)length) * 0.4 - 0.2) + position;
                buffer[i] = position;
            }
            break;

        case PULSE:
            for(i=0; i < length; i++) {
                if(i < pulse_length) {
                    position = (double)i / (double)pulse_length;
                    buffer[i] = WT_TRI(position, amp, phase, offset, period);
                } else {
                    buffer[i] = 0.0;
                }
            }
            break;

        case SQUARE:
            for(i=0; i < length; i++) {
                if(i < length / 2) {
                    buffer[i] = amp;
                } else {
                    buffer[i] = 0;
                }
            }
            break;

        case FLAT:
            for(i=0; i < length; i++) {
                buffer[i] = amp;
            }
            break;

        case SINE:
        default:
            for(i=0; i < length; i++) {
                position = (double)i / (double)(length);
                buffer[i] = WT_SINE(position, amp, phase, offset, period);
            }
    }

}

/* Saturate overflows to maximum or minimum
 *
 * TODO: Could use rethinking? There's 
 * probably a more handy way to handle it. Hand.
 */
static short saturate(double value) {
    if(value > MAXVAL) {
        value = MAXVAL - 1;
    } else if(value < MINVAL) {
        value = MINVAL + 1;
    }
    
    return (short)value;
}

/* Returns the byte width corresponding to the 
 * bit depth currently set as a property on the 
 * pippi.dsp module.
 */
static int getsize() {
    PyObject *pippi_dsp = PyImport_AddModule("pippi.dsp");
    PyObject *depth = PyObject_GetAttrString(pippi_dsp, "bitdepth");

    int bitdepth = (int)PyInt_AsLong(depth);
    Py_DECREF(depth);

    /* We're only interested in the width of the 
     * data being stored in char array buffers.
     */
    return bitdepth / 8;
}

/* Hermite interpolation
 *
 * Ripped directly from Ye Olde musicdsp.org archives. This implementation was 
 * credited to James "supercollider" McCartney.
 *
 * x is the position between the middle two samples - or fractional position
 * y0 is sample value before current
 * y1 is current sample
 * y2 is current sample +1
 * y3 is current sample +2
 */
static double hermite(double x, double y0, double y1, double y2, double y3) {
    double c0 = y1;
    double c1 = 0.5 * (y2 - y0);
    double c3 = 1.5 * (y1 - y2) + 0.5 * (y3 - y0);
    double c2 = y0 - y1 + c1 - c3;

    return ((c3 * x + c2) * x + c1) * x + c0;
}

static double get_double(int *input_buffer, int position) {
    short value = 0;

    value = *BUFFER(input_buffer, position);
    return value / (double)MAXVAL;
}

/*
 * Multiply a sound by a constant
 *
 * This function has hella comments that apply to most of the 
 * rest of the pippic functions. Explanations elsewhere are omitted
 * if this function serves to explain what's going on.
 */
static char pippic_amp_docstring[] = "Multiply a sound by a constant.";
static PyObject * pippic_amp(PyObject *self, PyObject *args) {
    /* A PyObject representation of the sound */
    PyObject *output;

    /* A signed char array which will be filled
     * with the sound input
     */
    signed char *input;

    /* This will become a pointer to the internal 
     * buffer of the input string.
     */
    signed char *data;

    /* Multiplication factor */
    double factor;

    /* Temporary storage for computed values */
    double scaled_value;

    /* Temporary storage for the input and output values */
    int value = 0;

    /* The length of the sound */
    int length;
    
    /* Position in the sound */
    int i;

    /* This is the byte width for the char array buffer */
    int size = getsize();

    /* Convert python arguments to C types.
     * This function expects an audio string and float. 
     * The former becomes a pointer to a char array filled
     * with input data along with its length, and the latter 
     * becomes a double.
     */
    if(!PyArg_ParseTuple(args, "s#d", &input, &length, &factor)) {
        return 0;
    }

    /* The PyObject instance that will be returned. */
    output = PyString_FromStringAndSize(NULL, length);

    /* Returns a pointer to the internal buffer of the 
     * PyObject output string. 
     */
    data = (signed char*)PyString_AsString(output);

    /* Loop over the sound and multiply each value by the factor. 
     */
    for (i=0; i < length; i += size) {
        value = *BUFFER(input, i);
        scaled_value = (double)value * factor;

        *BUFFER(data, i) = saturate(scaled_value);
    }

    return output;
}

static char pippic_curve_docstring[] = "Envelope and control curve generation.";
static PyObject * pippic_curve(PyObject *self, PyObject *args) {
    Py_ssize_t i;

    int type, length;
    double amp = 1.0;
    double phase = 0.0;
    double offset = 0.0;
    double period = 1.0;

    if(!PyArg_ParseTuple(args, "ii|dddd", &type, &length, &period, &amp, &phase, &offset)) {
        return NULL; 
    }

    double data[length];

    wavetable(type, data, length, amp, phase, offset, period);

    PyObject *output = PyList_New(length);
    PyObject *value;

    for(i=0; i < length; i++) {
        value = PyFloat_FromDouble(data[i]);
        PyList_SetItem(output, i, value);
    }

    return output;
}

static char pippic_cycle_docstring[] = "Generate a single cycle of a waveform.";
static PyObject * pippic_cycle(PyObject *self, PyObject *args) {
    PyObject *output;
    signed char *data;

    int i, type, length;
    short value;
    double amp = 1.0;
    double phase = 0.0;
    double offset = 0.0;
    double period = 1.0;
    double multiple = 1.0;

    if(!PyArg_ParseTuple(args, "ii|dddd", &type, &length, &multiple, &amp, &phase, &offset)) {
        return NULL;
    }

    int size = getsize();
    int channels = 2;
    int chunk = size + channels;
    double wtable[length];

    switch(type) {
        case SINE:
        case COS:
        case VARY:
        case HANN:
            period = M_PI * 2.0;
            break;
    }

    output = PyString_FromStringAndSize(NULL, length * chunk);
    data = (signed char*)PyString_AsString(output);

    wavetable(type, wtable, length, amp, phase, offset, period * multiple);

    for(i=0; i < length; i++) {
        value = saturate(wtable[i] * (MAXVAL - 1));
         
        *BUFFER(data, i * chunk) = value;
        *BUFFER(data, i * chunk + size) = value;
    }

    return output;
}

static char pippic_env_docstring[] = "Apply an envelope to a sound.";
static PyObject * pippic_env(PyObject *self, PyObject *args) {
    PyObject *output;
    signed char *data;
    signed char *input;

    Py_ssize_t i;

    int type, input_length;
    double amp = 1.0;
    double phase = 0.0;
    double offset = 0.0;
    double period = 1.0;
    double mult   = 1.0;

    if(!PyArg_ParseTuple(args, "s#i|dddd", &input, &input_length, &type, &amp, &phase, &offset, &mult)) {
        return NULL; 
    }

    int size = getsize();
    int channels = 2;
    int chunk = size + channels;
    double wtable[input_length / chunk];
    double value, left, right;

    switch(type) {
        case SINE:
        case COS:
        case VARY:
        case HANN:
            period = M_PI;
            break;
    }

    output = PyString_FromStringAndSize(NULL, input_length);
    data = (signed char*)PyString_AsString(output);

    wavetable(type, wtable, input_length / chunk, amp, phase, offset, period * mult);

    for(i=0; i < input_length; i += chunk) {
        value = wtable[i / chunk];

        left = (double)*BUFFER(input, i);
        left *= value;
        right = (double)*BUFFER(input, i + size);
        right *= value;

        *BUFFER(data, i) = saturate(left);
        *BUFFER(data, i + size) = saturate(right);
    }

    return output;
}


/* Synthy
 */
static char pippic_synth_docstring[] = "Synthy.";
static PyObject * pippic_synth(PyObject *self, PyObject *args) {
    PyObject *output;
    signed char *data;

    int type;
    double frequency;

    /* Defaults for optional args */
    double amp = 1.0;
    double phase = 0.0;
    double offset = 0.0;
    double period = 1.0;
    int length = 44100;
    double oversample = 32.0;

    int i;
    int size = getsize();

    if(!PyArg_ParseTuple(args, "id|idddd", &type, &frequency, &length, &amp, &phase, &offset, &period)) {
        return NULL;
    }

    int chunk = size + 2;
    length = length * chunk;

    if((type == SINE || type == COS || type == HANN) && period == 1.0) {
        period = M_PI * 2.0;
    }

    output = PyString_FromStringAndSize(NULL, length);
    data = (signed char*)PyString_AsString(output);

    if(type == RSAW || type == PULSE || type == SQUARE) {
        frequency *= oversample; /* TODO: fix period phase and offset on RSAW and PULSE */
    }

    int cycle_length = (int)((44100.0 / frequency) * oversample) * chunk;

    double cycle[cycle_length / chunk];
    wavetable(type, cycle, cycle_length / chunk, amp, phase, offset, period * oversample);

    int left, right;
    short value;

    for(i=0; i < length; i += chunk) {
        value = saturate(cycle[(i / chunk) % (cycle_length / chunk)] * (MAXVAL - 1));

        *BUFFER(data, i) = value;
        *BUFFER(data, i + size) = value;
    }

    return output;
}

static char pippic_time_docstring[] = "Returns monotonic time.";
static PyObject * pippic_time(PyObject *self, PyObject*args) {
    struct timespec now;
    long long seconds, offset_seconds = 0;
    long nanoseconds, offset_nanoseconds = 0;

    if(!PyArg_ParseTuple(args, "|Ll:mtime", &offset_seconds, &offset_nanoseconds)) {
        return NULL;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);

    seconds = (long long)now.tv_sec + offset_seconds;
    nanoseconds = now.tv_sec + offset_nanoseconds;

    return Py_BuildValue("Ll", seconds, nanoseconds);
}

/* Add two sounds together
 */
static char pippic_add_docstring[] = "Add two sounds together.";
static PyObject * pippic_add(PyObject *self, PyObject *args) {
    PyObject *output;
    signed char *data;

    signed char *first, *second;
    int first_length, first_value, second_length, second_value, length, sum;

    int i;
    int size = getsize();

    if(!PyArg_ParseTuple(args, "s#s#:add", &first, &first_length, &second, &second_length)) {
        return NULL;
    }

    /* The output length is equal to the length of the longest input sound */
    length = first_length > second_length ? first_length : second_length;

    output = PyString_FromStringAndSize(NULL, length);
    data = (signed char*)PyString_AsString(output);

    for(i=0; i < length; i += size) {
        if(i < first_length) {
            first_value = (int)*BUFFER(first, i);
        } else {
            first_value = 0;
        }

        if(i < second_length) {
            second_value = (int)*BUFFER(second, i);
        } else {
            second_value = 0;
        }

        sum = first_value + second_value;

        *BUFFER(data, i) = saturate((double)sum);
    }

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
static char pippic_mix_docstring[] = "Mix an arbitrary number of sounds together.";
static PyObject * pippic_mix(PyObject *self, PyObject *args) {
    PyObject *output;
    signed char *data;

    PyObject *sounds;

    /* Unused, but captured to avoid breaking the old API */
    PyObject *right_align;
    double factor;

    int i, f, summed_data, tmp_data = 0;
    int size = getsize();

    if(!PyArg_ParseTuple(args, "O!|Od:mix", &PyList_Type, &sounds, &right_align, &factor)) {
        return NULL;
    }

    Py_ssize_t numsounds = PyList_Size(sounds);
    PyObject *swap;
    signed char *swap_data;
    Py_ssize_t swap_length;

    PyList_Sort(sounds);

    swap = PyList_GET_ITEM(sounds, 0);
    Py_ssize_t maxlength = PyString_GET_SIZE(swap);

    output = PyString_FromStringAndSize(NULL, maxlength);
    data = (signed char*)PyString_AsString(output);

    /* Write zeros into the output buffer */
    for(f=0; f < maxlength; f += size) {
        *BUFFER(data, f) = (short)0;
    }

    /* Loop through each sound, and add to the output buffer. */
    for(i=0; i < numsounds; i++) {
        swap = PyList_GET_ITEM(sounds, i);
        swap_length = PyString_GET_SIZE(swap);
        swap_data = (signed char*)PyString_AsString(swap);

        for(f=0; f < maxlength; f += size) {
            if(f < swap_length) {
                /* Add to output buffer */
                summed_data = *BUFFER(data, f);
                tmp_data = *BUFFER(swap_data, f);
                summed_data = summed_data + tmp_data;

                *BUFFER(data, f) = saturate((double)summed_data);
            } else {
                continue;
            }
        }
    }

    return output;
}


/* Multiply two sounds together
 */
static char pippic_am_docstring[] = "Multiply two sounds together.";
static PyObject * pippic_am(PyObject *self, PyObject *args) {
    PyObject *output;
    signed char *data;

    signed char *carrier, *modulator;
    int carrier_length, modulator_length, length, carrier_value;
    double product, modulator_value;

    int i;
    int size = getsize();

    if(!PyArg_ParseTuple(args, "s#s#", &carrier, &carrier_length, &modulator, &modulator_length)) {
        return 0;
    }

    /* The output length is equal to the length of the longest input sound */
    length = carrier_length > modulator_length ? carrier_length : modulator_length;

    output = PyString_FromStringAndSize(NULL, length);
    data = (signed char*)PyString_AsString(output);

    for(i=0; i < length; i += size) {
        if(i < carrier_length) {
            carrier_value = (int)*BUFFER(carrier, i);
        } else {
            carrier_value = 0;
        }

        if(i < modulator_length) {
            modulator_value = (int)*BUFFER(modulator, i) / (double)MAXVAL;
        } else {
            modulator_value = 1.0;
        }

        product = carrier_value * modulator_value;

        *BUFFER(data, i) = saturate(product);
    }

    return output;
}

/*
 */
static char pippic_shift_docstring[] = "Change speed.";
static PyObject * pippic_shift(PyObject *self, PyObject *args) {
    PyObject *output;
    signed char *data, *input;

    int input_length, input_frames, output_length, left, right, chunk;
    double speed, output_frames, frame_chunks, frame_delta;

    int output_count = 0;
    int frame_count = 0;
    int num_changes = 0;
    int size = getsize();
    int i, p = 0;

    if(!PyArg_ParseTuple(args, "s#d", &input, &input_length, &speed)) {
        return 0;
    }

    /* Length of input in frames */
    input_frames = input_length / (size + 2);

    /* A chunk is just the number of elements in our buffer 
     * needed to store a complete frame. */
    chunk = size + 2;

    output = PyString_FromStringAndSize(NULL, output_length);
    data = (signed char*)PyString_AsString(output);


    /* always calculate position in frames. 
     * how do i ensure no frames are skipped? */

    /* Loop over the input buffer frame by frame. Copy frames into 
     * the output buffer. Duplicate frames when we need to
     * insert an extra frame, or skip frames when we need to 
     * remove them.
     */
    for(i=0; i < output_length; i += chunk) {
        /*p = 0;*/
        /*left = get_double(input, p);*/
        /*right = get_double(input, p + size);*/

        /**BUFFER(data, i) = saturate(left);*/
        /**BUFFER(data, i + size) = saturate(right);*/

        *BUFFER(data, i) = saturate(0.0);
        *BUFFER(data, i + size) = saturate(0.0);
    }

    return output;
}

static PyMethodDef pippic_methods[] = {
    {"amp", pippic_amp, METH_VARARGS, pippic_amp_docstring},
    {"am", pippic_am, METH_VARARGS, pippic_am_docstring},
    {"add", pippic_add, METH_VARARGS, pippic_add_docstring},
    {"mix", pippic_mix, METH_VARARGS, pippic_mix_docstring},
    {"shift", pippic_shift, METH_VARARGS, pippic_shift_docstring},
    {"synth", pippic_synth, METH_VARARGS, pippic_synth_docstring},
    {"mtime", pippic_time, METH_VARARGS, pippic_time_docstring},
    {"pine", pippic_pine, METH_VARARGS, pippic_pine_docstring},
    {"curve", pippic_curve, METH_VARARGS, pippic_curve_docstring},
    {"env", pippic_env, METH_VARARGS, pippic_env_docstring},
    {"cycle", pippic_cycle, METH_VARARGS, pippic_cycle_docstring},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_pippic(void) {
    (void) Py_InitModule("_pippic", pippic_methods);
}
