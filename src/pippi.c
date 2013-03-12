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

/* Saturate overflows to maximum or minimum
 */
static short pp_saturate(double value) {
    if(value > MAXVAL) {
        value = MAXVAL;
    } else if(value < MINVAL) {
        value = MINVAL;
    }
    
    return (short)value;
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

    /* This is the byte width for a 16 bit integer */
    int size = 2;

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

        *BUFFER(data, i) = pp_saturate(scaled_value);
    }

    return output;
}

/* Synthesize a sinewave
 */
static char pippic_sine_docstring[] = "Synthesize a sinewave.";
static PyObject * pippic_sine(PyObject *self, PyObject *args, PyObject *keywords) {
    PyObject *output;
    signed char *data;

    /* Keyword arguments */
    static char *keyword_list[] = {"freq", "len", "amp", NULL};

    /* Defaults for optional args */
    double amplitude = 1.0;
    int length = 44100;

    double frequency, position, period, value;
    int i, chunk;
    int size = 2;

    if(!PyArg_ParseTupleAndKeywords(args, keywords, "d|id", keyword_list, &frequency, &length, &amplitude)) {
        return 0;
    }

    chunk = size + 2;
    length = length * chunk;

    output = PyString_FromStringAndSize(NULL, length);
    data = (signed char*)PyString_AsString(output);

    period = M_PI * 2.0;
    for(i=0; i < length; i += chunk) {
        position = (double)i / (double)length;
        value = sin(position * period * frequency) * amplitude;

        /* Scale to signed 16 bit integer */
        value = value * (double)MAXVAL;
        value = pp_saturate(value);

        /* Write to the left channel */
        *BUFFER(data, i) = value;

        /* Write to the right channel */
        *BUFFER(data, i + size) = value;
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
    int size = 2;

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

        *BUFFER(data, i) = pp_saturate(product);
    }

    return output;
}

/*
 * Add two sounds together
 */
static PyObject * pippic_add(PyObject *self, PyObject *args) {

}

/*
 * Sum two sounds, mixed with equal power
 */
static PyObject * pippic_mix(PyObject *self, PyObject *args) {

}

/*
 * Apply an amplitude function to a sound
 */
static PyObject * pippic_env(PyObject *self, PyObject *args) {

}

static PyMethodDef pippic_methods[] = {
    {"amp", pippic_amp, METH_VARARGS, pippic_amp_docstring},
    {"am", pippic_am, METH_VARARGS, pippic_am_docstring},
    {"sine", pippic_sine, METH_VARARGS | METH_KEYWORDS, pippic_sine_docstring},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_pippic(void) {
    (void) Py_InitModule("_pippic", pippic_methods);
}
