#include "pippicore.h"

#define LOGISTIC_SEED_DEFAULT 3.999
#define LOGISTIC_X_DEFAULT 0.555

#define LORENZ_TIMESTEP_DEFAULT 0.01
#define LORENZ_A_DEFAULT 10.0
#define LORENZ_B_DEFAULT 28.0
#define LORENZ_C_DEFAULT (8.0 / 3.0)
#define LORENZ_X_DEFAULT 0.1
#define LORENZ_Y_DEFAULT 0.0
#define LORENZ_Z_DEFAULT 0.0

#define PLOT_WIDTH 80


/* Forward declarations */
void rand_preseed(void);
void rand_seed(int value);
lpfloat_t rand_base_logistic(lpfloat_t low, lpfloat_t high);
lpfloat_t rand_base_stdlib(lpfloat_t low, lpfloat_t high);
lpfloat_t rand_rand(lpfloat_t low, lpfloat_t high);
lpfloat_t rand_base_lorenz(lpfloat_t low, lpfloat_t high);
lpfloat_t rand_base_lorenzX(lpfloat_t low, lpfloat_t high);
lpfloat_t rand_base_lorenzY(lpfloat_t low, lpfloat_t high);
lpfloat_t rand_base_lorenzZ(lpfloat_t low, lpfloat_t high);
int rand_randint(int low, int high);
int rand_randbool(void);
int rand_choice(int numchoices);

lparray_t * create_array_from(int numvalues, ...);
lparray_t * create_array(size_t length);
void destroy_array(lparray_t * array);

lpbuffer_t * create_buffer(size_t length, int channels, int samplerate);
lpbuffer_t * create_buffer_from_float(lpfloat_t value, size_t length, int channels, int samplerate);
lpstack_t * create_uniform_stack(int numbuffers, size_t buffer_length, int channels, int samplerate);
void copy_buffer(lpbuffer_t * src, lpbuffer_t * dest);
void clear_buffer(lpbuffer_t * buf);
void scale_buffer(lpbuffer_t * buf, lpfloat_t from_min, lpfloat_t from_max, lpfloat_t to_min, lpfloat_t to_max);
lpfloat_t min_buffer(lpbuffer_t * buf);
lpfloat_t max_buffer(lpbuffer_t * buf);
lpfloat_t mag_buffer(lpbuffer_t * buf);
void multiply_buffer(lpbuffer_t * a, lpbuffer_t * b);
void scalar_multiply_buffer(lpbuffer_t * a, lpfloat_t b);
void add_buffers(lpbuffer_t * a, lpbuffer_t * b);
void scalar_add_buffer(lpbuffer_t * a, lpfloat_t b);
void subtract_buffers(lpbuffer_t * a, lpbuffer_t * b);
void scalar_subtract_buffer(lpbuffer_t * a, lpfloat_t b);
void divide_buffers(lpbuffer_t * a, lpbuffer_t * b);
void scalar_divide_buffer(lpbuffer_t * a, lpfloat_t b);
lpbuffer_t * concat_buffers(lpbuffer_t * a, lpbuffer_t * b);
int buffers_are_equal(lpbuffer_t * a, lpbuffer_t * b);
int buffers_are_close(lpbuffer_t * a, lpbuffer_t * b, int d);
void dub_buffer(lpbuffer_t * a, lpbuffer_t * b, size_t start);
void dub_scalar(lpbuffer_t * a, lpfloat_t, size_t start);
void env_buffer(lpbuffer_t * buf, lpbuffer_t * env);
void plot_buffer(lpbuffer_t * buf);
lpfloat_t play_buffer(lpbuffer_t * buf, lpfloat_t speed);
void copy_buffer(lpbuffer_t * src, lpbuffer_t * dest);
void split2_buffer(lpbuffer_t * src, lpbuffer_t * a, lpbuffer_t * b);
lpbuffer_t * mix_buffers(lpbuffer_t * a, lpbuffer_t * b);
lpbuffer_t * remix_buffer(lpbuffer_t * buf, int channels);
lpbuffer_t * cut_buffer(lpbuffer_t * buf, size_t start, size_t length);
void cut_into_buffer(lpbuffer_t * buf, lpbuffer_t * out, size_t start, size_t length);
lpbuffer_t * resample_buffer(lpbuffer_t * buf, size_t length);
void pan_buffer(lpbuffer_t * buf, lpbuffer_t * pos);
lpbuffer_t * resize_buffer(lpbuffer_t *, size_t);
void destroy_buffer(lpbuffer_t * buf);
void destroy_stack(lpstack_t * stack);

lpfloat_t read_skewed_buffer(lpfloat_t freq, lpbuffer_t * buf, lpfloat_t phase, lpfloat_t skew);
lpfloat_t fx_lpf1(lpfloat_t x, lpfloat_t * y, lpfloat_t cutoff, lpfloat_t samplerate);
void fx_convolve(lpbuffer_t * a, lpbuffer_t * b, lpbuffer_t * out);
void fx_norm(lpbuffer_t * buf, lpfloat_t ceiling);

lpbuffer_t * ringbuffer_create(size_t length, int channels, int samplerate);
void ringbuffer_fill(lpbuffer_t * ringbuf, lpbuffer_t * buf, int offset);
lpfloat_t ringbuffer_readone(lpbuffer_t * ringbuf, int offset);
lpbuffer_t * ringbuffer_read(lpbuffer_t * ringbuf, size_t length);
void ringbuffer_readinto(lpbuffer_t * ringbuf, lpfloat_t * data, size_t length, int channels);
void ringbuffer_writeone(lpbuffer_t * ringbuf, lpfloat_t sample);
void ringbuffer_writefrom(lpbuffer_t * ringbuf, lpfloat_t * data, size_t length, int channels);
void ringbuffer_write(lpbuffer_t * ringbuf, lpbuffer_t * buf);
void ringbuffer_dub(lpbuffer_t * buf, lpbuffer_t * src);
void ringbuffer_destroy(lpbuffer_t * buf);

void memorypool_init(unsigned char * pool, size_t poolsize);
lpmemorypool_t * memorypool_custom_init(unsigned char * pool, size_t poolsize);
void * memorypool_alloc(size_t itemcount, size_t itemsize);
void * memorypool_custom_alloc(lpmemorypool_t * pool, size_t itemcount, size_t itemsize);
void memorypool_free(void * ptr);

lpfloat_t interpolate_hermite(lpbuffer_t * buf, lpfloat_t phase);
lpfloat_t interpolate_hermite_pos(lpbuffer_t * buf, lpfloat_t pos);
lpfloat_t interpolate_linear(lpbuffer_t * buf, lpfloat_t phase);
lpfloat_t interpolate_linear_pos(lpbuffer_t * buf, lpfloat_t pos);
lpfloat_t interpolate_linear_channel(lpbuffer_t* buf, lpfloat_t phase, int channel);

lpbuffer_t * param_create_from_float(lpfloat_t value);
lpbuffer_t * param_create_from_int(int value);

lpbuffer_t * create_wavetable(int name, size_t length);
lpstack_t * create_wavetable_stack(int numtables, ...);
void destroy_wavetable(lpbuffer_t* buf);
lpbuffer_t* create_window(int name, size_t length);
lpstack_t * create_window_stack(int numtables, ...);
void destroy_window(lpbuffer_t* buf);

/* Populate interfaces */
lprand_t LPRand = { LOGISTIC_SEED_DEFAULT, LOGISTIC_X_DEFAULT, \
    LORENZ_TIMESTEP_DEFAULT, \
    LORENZ_X_DEFAULT, LORENZ_Y_DEFAULT, LORENZ_Z_DEFAULT, \
    LORENZ_A_DEFAULT, LORENZ_B_DEFAULT, LORENZ_C_DEFAULT, \
    rand_preseed, rand_seed, rand_base_stdlib, rand_base_logistic, \
    rand_base_lorenz, rand_base_lorenzX, rand_base_lorenzY, rand_base_lorenzZ, \
    rand_base_stdlib, rand_rand, rand_randint, rand_randbool, rand_choice };
lpmemorypool_factory_t LPMemoryPool = { 0, 0, 0, memorypool_init, memorypool_custom_init, memorypool_alloc, memorypool_custom_alloc, memorypool_free };
const lparray_factory_t LPArray = { create_array, create_array_from, destroy_array };
const lpbuffer_factory_t LPBuffer = { create_buffer, create_buffer_from_float, create_uniform_stack, copy_buffer, clear_buffer, split2_buffer, scale_buffer, min_buffer, max_buffer, mag_buffer, play_buffer, pan_buffer, mix_buffers, remix_buffer, cut_buffer, cut_into_buffer, resample_buffer, multiply_buffer, scalar_multiply_buffer, add_buffers, scalar_add_buffer, subtract_buffers, scalar_subtract_buffer, divide_buffers, scalar_divide_buffer, concat_buffers, buffers_are_equal, buffers_are_close, dub_buffer, dub_scalar, env_buffer, resize_buffer, plot_buffer, destroy_buffer, destroy_stack };
const lpinterpolation_factory_t LPInterpolation = { interpolate_linear_pos, interpolate_linear, interpolate_linear_channel, interpolate_hermite_pos, interpolate_hermite };
const lpparam_factory_t LPParam = { param_create_from_float, param_create_from_int };
const lpwavetable_factory_t LPWavetable = { create_wavetable, create_wavetable_stack, destroy_wavetable };
const lpwindow_factory_t LPWindow = { create_window, create_window_stack, destroy_window };
const lpringbuffer_factory_t LPRingBuffer = { ringbuffer_create, ringbuffer_fill, ringbuffer_read, ringbuffer_readinto, ringbuffer_writefrom, ringbuffer_write, ringbuffer_readone, ringbuffer_writeone, ringbuffer_dub, ringbuffer_destroy };
const lpfx_factory_t LPFX = { read_skewed_buffer, fx_lpf1, fx_convolve, fx_norm };

/* Platform-specific random seed, called 
 * on program init (and on process pool init) 
 * from python or optionally elsewhere to 
 * seed random with nice bytes. */
void rand_preseed() {
#ifdef __linux__
    unsigned int * buffer;
    ssize_t bytes_read;
    size_t buffer_size;
    buffer_size = sizeof(unsigned int);
    buffer = LPMemoryPool.alloc(1, buffer_size);
    bytes_read = getrandom(buffer, buffer_size, 0);
    if(bytes_read > 0) srand(*buffer);
    free(buffer);
#endif
}

/* User rand seed */
void rand_seed(int value) {
    srand((unsigned int)value);
}

/* Default rand_base callback. 
 *
 * These base rand_base callbacks return 0-1 and 
 * are the basis of all other rand functions like 
 * choice and randint.
 *
 * They may be swapped out at runtime by setting 
 * LPRand.rand_base to the desired rand_base pointer;
 * */
lpfloat_t rand_base_stdlib(lpfloat_t low, lpfloat_t high) {
    return (rand()/(lpfloat_t)RAND_MAX) * (high-low) + low;
}

/* Logistic rand base. */
lpfloat_t rand_base_logistic(lpfloat_t low, lpfloat_t high) {
    LPRand.logistic_x = LPRand.logistic_seed * LPRand.logistic_x * (1.f - LPRand.logistic_x);
    return LPRand.logistic_x * (high-low) + low;
}

/* The three Lorenz attractor implementations (lorenzX, lorenzY, lorenzZ) 
 * were lightly adapted with permission from Greg Cope's helpful overview: 
 *      https://www.algosome.com/articles/lorenz-attractor-programming-code.html
 * Please consider those routines to be included here under an MIT license.
 */
lpfloat_t lorenzX(lpfloat_t low, lpfloat_t high) {
    LPRand.lorenz_x = LPRand.lorenz_x + LPRand.lorenz_timestep * LPRand.lorenz_a * (LPRand.lorenz_y - LPRand.lorenz_x);
    return LPRand.lorenz_x * (high-low) + low;
}

lpfloat_t lorenzY(lpfloat_t low, lpfloat_t high) {
    LPRand.lorenz_y = LPRand.lorenz_y + LPRand.lorenz_timestep * (LPRand.lorenz_x * (LPRand.lorenz_b - LPRand.lorenz_z) - LPRand.lorenz_y);
    return LPRand.lorenz_y * (high-low) + low;
}

lpfloat_t lorenzZ(lpfloat_t low, lpfloat_t high) {
    LPRand.lorenz_z = LPRand.lorenz_z + LPRand.lorenz_timestep * (LPRand.lorenz_x * LPRand.lorenz_y - LPRand.lorenz_c * LPRand.lorenz_z);
    return LPRand.lorenz_z * (high-low) + low;
}

lpfloat_t rand_base_lorenzX(lpfloat_t low, lpfloat_t high) {
    lpfloat_t x;
    x = lorenzX(0.f, 1.f);
    lorenzY(0.f, 1.f);
    lorenzZ(0.f, 1.f);
    return x * (high-low) + low;
}

lpfloat_t rand_base_lorenzY(lpfloat_t low, lpfloat_t high) {
    lpfloat_t y;
    lorenzX(0.f, 1.f);
    y = lorenzY(0.f, 1.f);
    lorenzZ(0.f, 1.f);
    return y * (high-low) + low;
}

lpfloat_t rand_base_lorenzZ(lpfloat_t low, lpfloat_t high) {
    lpfloat_t z;
    lorenzX(0.f, 1.f);
    lorenzY(0.f, 1.f);
    z = lorenzZ(0.f, 1.f);
    return z * (high-low) + low;
}

lpfloat_t rand_base_lorenz(lpfloat_t low, lpfloat_t high) {
    lpfloat_t x, y, z;
    x = lorenzX(0.f, 1.f);
    y = lorenzY(0.f, 1.f);
    z = lorenzZ(0.f, 1.f);
    return x * y * z * (high-low) + low;
}

lpfloat_t rand_rand(lpfloat_t low, lpfloat_t high) {
    return LPRand.rand_base(low, high);
}

int rand_randint(int low, int high) {
    float diff, tmp;

    tmp = (float)rand_rand((lpfloat_t)low, (lpfloat_t)high);
    diff = (int)tmp - tmp;

    if(diff >= 0.5f) {
        return (int)ceil(tmp);
    } else {
        return (int)floor(tmp);
    }
}

int rand_randbool(void) {
    return rand_randint(0, 1);
}

int rand_choice(int numchoices) {
    assert(numchoices > 0);
    if(numchoices == 1) return 0;
    return rand_randint(0, numchoices-1);
}

lparray_t * create_array(size_t length) {
    size_t i = 0;
    lparray_t * array = (lparray_t*)LPMemoryPool.alloc(1, sizeof(lparray_t));
    array->data = (int*)LPMemoryPool.alloc(length, sizeof(int));
    array->length = length;
    for(i=0; i < array->length; i++) {
        array->data[i] = 0;
    }
    array->phase = 0.f;
    return array;
}

lparray_t * create_array_from(int numvalues, ...) {
    va_list vl;
    lparray_t * array;
    int i;

    va_start(vl, numvalues);

    array = (lparray_t*)LPMemoryPool.alloc(1, sizeof(lparray_t));
    array->data = (int*)LPMemoryPool.alloc(numvalues, sizeof(int));

    for(i=0; i < numvalues; i++) {
        array->data[i] = va_arg(vl, int);
    }

    va_end(vl);

    return array;
}

void destroy_array(lparray_t * array) {
    if(array != NULL) {
        LPMemoryPool.free(array->data);
        LPMemoryPool.free(array);
    }
}

/* Buffer
 * */
lpbuffer_t * create_buffer(size_t length, int channels, int samplerate) {
    lpbuffer_t * buf;
    buf = (lpbuffer_t*)LPMemoryPool.alloc(1, sizeof(lpbuffer_t));
    buf->data = (lpfloat_t*)LPMemoryPool.alloc(length * channels, sizeof(lpfloat_t));
    buf->channels = channels;
    buf->length = length;
    buf->samplerate = samplerate;
    buf->phase = 0.f;
    buf->pos = 0;
    buf->boundry = length-1;
    buf->range = length;
    buf->onset = 0;
    buf->is_looping = 0;
    return buf;
}

lpbuffer_t * create_buffer_from_float(lpfloat_t value, size_t length, int channels, int samplerate) {
    size_t i;
    int c;
    lpbuffer_t * buf;
    buf = create_buffer(length, channels, samplerate);
    for(i=0; i < length; i++) {
        for(c=0; c < channels; c++) {
            buf->data[i * channels + c] = value;
        }
    }
    return buf;
}

/*
void stack2_buffer(lpbuffer_t * src, lpbuffer_t * a, lpbuffer_t * b) {
    size_t i;

    assert(src->channels == 2);

    a = LPBuffer.create(src->length, 1, src->samplerate);
    b = LPBuffer.create(src->length, 1, src->samplerate);

    for(i=0; i < src->length; i++) {
        a->data[i] = src->data[i * 2];        
        a->data[i * 2] = src->data[i * 2 + 1];        
    }
}
*/


void split2_buffer(lpbuffer_t * src, lpbuffer_t * a, lpbuffer_t * b) {
    size_t i;

    printf("src length: %d\n  a length: %d\n  b length: %d\n", (int)src->length, (int)a->length, (int)b->length);

    assert(src->channels == 2);
    assert(src->length == a->length);
    assert(src->length == b->length);

    for(i=0; i < src->length; i++) {
        a->data[i] = src->data[i * 2];        
        b->data[i] = src->data[i * 2 + 1];        
    }
}

void copy_buffer(lpbuffer_t * src, lpbuffer_t * dest) {
    size_t i;
    int c;

    assert(src->length == dest->length);
    assert(src->channels == dest->channels);

    for(i=0; i < src->length; i++) {
        for(c=0; c < src->channels; c++) {
            dest->data[i * src->channels + c] = src->data[i * src->channels + c];
        }
    }
}

void clear_buffer(lpbuffer_t * buf) {
    size_t i;
    int c;

    for(i=0; i < buf->length; i++) {
        for(c=0; c < buf->channels; c++) {
            buf->data[i * buf->channels + c] = 0.f;
        }
    }
}

void scale_buffer(lpbuffer_t * buf, lpfloat_t from_min, lpfloat_t from_max, lpfloat_t to_min, lpfloat_t to_max) {
    size_t i;
    int c, idx;
    lpfloat_t from_diff, to_diff;

    to_diff = to_max - to_min;;
    from_diff = from_max - from_min;;

    /* Maybe this is valid? It's weird to "scale" 
     * a buffer filled with one value, but I guess 
     * that's a case we should support...
     * Ideally we'll figure out how to get rid of that 
     * repeating divide and use an approach that supports 
     * this case.
     * Anyway: TODO handle this better?
     */
    assert(from_diff != 0);

    for(i=0; i < buf->length; i++) {
        for(c=0; c < buf->channels; c++) {
            idx = i * buf->channels + c;
            buf->data[idx] = ((buf->data[idx] - from_min) / from_diff) * to_diff + to_min;
        }
    }
}

lpfloat_t min_buffer(lpbuffer_t * buf) {
    lpfloat_t out = 0.f;
    size_t i;
    int c;

    for(i=0; i < buf->length; i++) {
        for(c=0; c < buf->channels; c++) {
            out = lpfmin(buf->data[i * buf->channels + c], out);
        }
    }
    return out;
}

lpfloat_t max_buffer(lpbuffer_t * buf) {
    lpfloat_t out = 0.f;
    size_t i;
    int c;

    for(i=0; i < buf->length; i++) {
        for(c=0; c < buf->channels; c++) {
            out = lpfmax(buf->data[i * buf->channels + c], out);
        }
    }
    return out;
}

lpfloat_t mag_buffer(lpbuffer_t * buf) {
    lpfloat_t out = 0.f;
    size_t i;
    int c;

    for(i=0; i < buf->length; i++) {
        for(c=0; c < buf->channels; c++) {
            out = lpfmax(fabs(buf->data[i * buf->channels + c]), out);
        }
    }
    return out;
}

void pan_buffer(lpbuffer_t * buf, lpbuffer_t * pos) {
    size_t i;
    int c;
    for(i=0; i < buf->length; i++) {
        for(c=0; c < buf->channels-1; c += 2) {
            buf->data[i * buf->channels + c] *= (lpfloat_t)sqrt(interpolate_linear_pos(pos, pos->phase));
            buf->data[i * buf->channels + c+1] *= (lpfloat_t)sqrt(1.f-interpolate_linear_pos(pos, pos->phase));
        }
    }
}

lpfloat_t play_buffer(lpbuffer_t * buf, lpfloat_t speed) {
    lpfloat_t phase_inc, value;
    phase_inc = 1.f / (buf->length * (1.f / speed));
    value = interpolate_linear_pos(buf, buf->phase);
    buf->phase += phase_inc;
    return value;
}

lpbuffer_t * resample_buffer(lpbuffer_t * buf, size_t length) {
    lpbuffer_t * out;
    lpfloat_t pos;
    size_t i;
    int c;

    assert(length > 1);
    out = create_buffer(length, buf->channels, buf->samplerate);
    for(i=0; i < length; i++) {
        pos = (lpfloat_t)i/length;
        for(c=0; c < buf->channels; c++) {
            out->data[i * buf->channels + c] = interpolate_linear_channel(buf, pos * buf->length, c);
        }
    }

    return out;
}

void multiply_buffer(lpbuffer_t * a, lpbuffer_t * b) {
    size_t length, i;
    int c, j;
    length = (a->length <= b->length) ? a->length : b->length;
    for(i=0; i < length; i++) {
        for(c=0; c < a->channels; c++) {
            j = c % b->channels;
            a->data[i * a->channels + c] *= b->data[i * b->channels + j];
        }
    }
}

void scalar_multiply_buffer(lpbuffer_t * a, lpfloat_t b) {
    size_t i;
    int c;
    for(i=0; i < a->length; i++) {
        for(c=0; c < a->channels; c++) {
            a->data[i * a->channels + c] *= b;
        }
    }
}

lpbuffer_t * concat_buffers(lpbuffer_t * a, lpbuffer_t * b) {
    size_t length, i;
    int c, channels;
    lpbuffer_t * out;

    length = a->length + b->length;
    channels = a->channels;
    out = create_buffer(length, channels, a->samplerate);

    for(i=0; i < a->length; i++) {
        for(c=0; c < channels; c++) {
            out->data[i * channels + c] = a->data[i * channels + c];
        }
    }

    for(i=a->length; i < length; i++) {
        for(c=0; c < channels; c++) {
            out->data[i * channels + c] = b->data[(i-a->length) * channels + c];
        }
    }

    return out;
}

void add_buffers(lpbuffer_t * a, lpbuffer_t * b) {
    size_t length, i;
    int c, j;
    length = (a->length <= b->length) ? a->length : b->length;
    for(i=0; i < length; i++) {
        for(c=0; c < a->channels; c++) {
            j = c % b->channels;
            a->data[i * a->channels + c] += b->data[i * b->channels + j];
        }
    }
}

void scalar_add_buffer(lpbuffer_t * a, lpfloat_t b) {
    size_t i;
    int c;
    for(i=0; i < a->length; i++) {
        for(c=0; c < a->channels; c++) {
            a->data[i * a->channels + c] += b;
        }
    }
}

void subtract_buffers(lpbuffer_t * a, lpbuffer_t * b) {
    size_t length, i;
    int c, j;
    length = (a->length <= b->length) ? a->length : b->length;
    for(i=0; i < length; i++) {
        for(c=0; c < a->channels; c++) {
            j = c % b->channels;
            a->data[i * a->channels + c] -= b->data[i * b->channels + j];
        }
    }
}

void scalar_subtract_buffer(lpbuffer_t * a, lpfloat_t b) {
    size_t i;
    int c;
    for(i=0; i < a->length; i++) {
        for(c=0; c < a->channels; c++) {
            a->data[i * a->channels + c] -= b;
        }
    }
}

void divide_buffers(lpbuffer_t * a, lpbuffer_t * b) {
    size_t length, i;
    int c, j;
    length = (a->length <= b->length) ? a->length : b->length;
    for(i=0; i < length; i++) {
        for(c=0; c < a->channels; c++) {
            j = c % b->channels;
            if(b->data[i * b->channels + j] == 0) {
                a->data[i * a->channels + c] = 0.f;
            } else {
                a->data[i * a->channels + c] /= b->data[i * b->channels + j];
            }
        }
    }
}

void scalar_divide_buffer(lpbuffer_t * a, lpfloat_t b) {
    size_t i;
    int c;
    if(b == 0) {
        for(i=0; i < a->length; i++) {
            for(c=0; c < a->channels; c++) {
                a->data[i * a->channels + c] = 0.f;
            }
        }
    } else {
        for(i=0; i < a->length; i++) {
            for(c=0; c < a->channels; c++) {
                a->data[i * a->channels + c] /= b;
            }
        }
    }
}

int buffers_are_equal(lpbuffer_t * a, lpbuffer_t * b) {
    size_t i;
    int c;
    if(a->length != b->length) return 0;
    if(a->channels != b->channels) return 0;
    for(i=0; i < a->length; i++) {
        for(c=0; c < a->channels; c++) {
            if(a->data[i * a->channels + c] != b->data[i * a->channels + c]) return 0;
        }
    }
    return 1;
}

int buffers_are_close(lpbuffer_t * a, lpbuffer_t * b, int d) {
    size_t i;
    int c;
    long atmp, btmp;
    if(a->length != b->length) return 0;
    if(a->channels != b->channels) return 0;
    for(i=0; i < a->length; i++) {
        for(c=0; c < a->channels; c++) {
            atmp = floor(a->data[i * a->channels + c] * d);
            btmp = floor(b->data[i * a->channels + c] * d);
            if(atmp != btmp) return 0;
        }
    }
    return 1;
}


void env_buffer(lpbuffer_t * buf, lpbuffer_t * env) {
    lpfloat_t pos, value;
    size_t i;
    int c;

    assert(env->length > 0);
    assert(env->channels == 1);

    for(i=0; i < buf->length; i++) {
        pos = (lpfloat_t)i / buf->length;
        value = interpolate_linear_pos(env, pos);
        for(c=0; c < buf->channels; c++) {
            buf->data[i * buf->channels + c] *= value;
        }
    }
}

wchar_t get_grid_char(int pixels[8]) {
    int i, r;
    int map[8] = {0,2,4,1,3,5,6,7};

    /* Braille dots are indexed like this:
     *   1 4
     *   2 5
     *   3 6 
     *   7 8
     *
     * Mapped to pixel inputs:
     *
     *   pixels:   0  1  2  3  4  5  6  7
     *   braille:  0  2  4  1  3  5  6  7
     *   byte idx: 1  2  4  8  16 32 64 128
     */

    r = 0;
    for(i=0; i < 8; i++) {
        if(pixels[map[i]] == 1) {
            r += (int)exp2(i);
        }
    }

    return (wchar_t)(GRID_EMPTY + r);
}

void plot_buffer(lpbuffer_t * buf) {
    int c, height, width, x, y;
    size_t i, pos, blocksize;
    float sample, value, peak, low, tpeak, tlow;
    int grid[PLOT_WIDTH];
    int peaks[PLOT_WIDTH];
    int lows[PLOT_WIDTH];
    float avgs[PLOT_WIDTH] = {0};
    int current, lastsign;
    int color;
    size_t crossing_count;
    int pixels[8] = {0,1,1,1,0,0,0,0};

    height = 10;
    width  = PLOT_WIDTH;

    blocksize = (size_t)(buf->length / (float)width);

    printf("block: %lc \n\n", get_grid_char(pixels));
    printf("\n\n");

    pos = 0;
    x = 0;
    tpeak = 0;
    tlow = 0;
    while(pos <= buf->length-blocksize) {
        peak = 0;
        low = 0;

        crossing_count = 0;
        lastsign = 0;
        for(i=0; i < blocksize; i++) {
            sample = 0.f;
            for(c=0; c < buf->channels; c++) {
                sample += (float)buf->data[(i+pos) * buf->channels + c];
                tpeak = fmax(tpeak, (float)buf->data[(i+pos) * buf->channels + c]);
                tlow = fmin(tlow, (float)buf->data[(i+pos) * buf->channels + c]);
            }
            value = (sample + 1) / 2.f;
            if(i < 25) avgs[x] += value;

            peak = fmax(peak, sample);
            low = fmin(low, sample);

            current = signbit(sample); 
            if((lastsign && !current) || (!lastsign && current)) {
                crossing_count += 1;
            }
            lastsign = current;
        }

        grid[x] = crossing_count;
        peaks[x] = (int)(((peak+1)/2.f) * height);
        lows[x] = (int)(((low+1)/2.f) * height);
        avgs[x] = avgs[x] / 25;

        pos += blocksize;
        x += 1;
    }

    for(y=0; y < height; y++) {
        for(x=0; x < width; x++) {
            if(y > peaks[x] || y < lows[x]) {
                printf(" ");
                continue;
            }

            crossing_count = grid[x];
            //color = (int)((crossing_count / 25.f) * 23) + 232;
            color = (int)(avgs[x] * 23) + 232;
            printf("\033[38;5;%dm", color);
            if(crossing_count < 5) {
                printf("-");
            } else if (crossing_count >= 5 && crossing_count < 10) {
                printf("+");
            } else if (crossing_count >= 10 && crossing_count < 15) {
                printf("+");
            } else if (crossing_count >= 15 && crossing_count < 20) {
                printf("#");
            } else if (crossing_count >= 20) {
                printf("#");
            }
            /*printf("%lc", (wchar_t)(start + grid[x][y]));*/
        } 
        printf("\n");
    }
    printf("\033[0m");
}

void dub_buffer(lpbuffer_t * a, lpbuffer_t * b, size_t start) {
    size_t i;
    int c, j;
    lpfloat_t sample;

    assert(start + b->length <= a->length);
    assert(b->length <= a->length);
    assert(a->channels == b->channels);

    for(i=0; i < b->length; i++) {
        for(c=0; c < a->channels; c++) {
            j = c % b->channels;
            sample = b->data[i * b->channels + j];
            a->data[(i+start) * a->channels + c] += sample;
        }
    }
}

void dub_scalar(lpbuffer_t * a, lpfloat_t val, size_t start) {
    int c;

    assert(start < a->length);
    for(c=0; c < a->channels; c++) {
        a->data[start * a->channels + c] += val;
    }
}

void cut_into_buffer(lpbuffer_t * buf, lpbuffer_t * out, size_t start, size_t length) {
    size_t i, writelength;
    int c;

    /* FIXME support zero-length buffers */
    assert(length > 0);

    if(start < buf->length) {
        writelength = buf->length - start;
        writelength = (writelength > length) ? length : writelength;
        for(i=0; i < writelength; i++) {
            for(c=0; c < buf->channels; c++) {
                out->data[i * buf->channels + c] = buf->data[(i+start) * buf->channels + c];
            }
        }
    }
}


lpbuffer_t * cut_buffer(lpbuffer_t * buf, size_t start, size_t length) {
    lpbuffer_t * out;

    /* FIXME support zero-length buffers */
    assert(length > 0);

    out = LPBuffer.create(length, buf->channels, buf->samplerate);
    cut_into_buffer(buf, out, start, length);
    return out;
}

lpbuffer_t * mix_buffers(lpbuffer_t * a, lpbuffer_t * b) {
    int max_channels, max_samplerate, c;
    size_t i;
    lpbuffer_t * out;
    lpbuffer_t * longest;
    lpbuffer_t * shortest;

    if(a->length >= b->length) {
        longest=a; shortest=b;
    } else {
        longest=b; shortest=a;
    }

    max_channels = (a->channels >= b->channels) ? a->channels : b->channels;
    max_samplerate = (a->samplerate >= b->samplerate) ? a->samplerate : b->samplerate;
    out = LPBuffer.create(longest->length, max_channels, max_samplerate);

    for(i=0; i < longest->length; i++) {
        for(c=0; c < max_channels; c++) {
            out->data[i * max_channels + c] = longest->data[i * max_channels + c];
        }
    }

    for(i=0; i < shortest->length; i++) {
        for(c=0; c < max_channels; c++) {
            out->data[i * max_channels + c] = shortest->data[i * max_channels + c];
        }
    }

    return out;
}

lpbuffer_t * remix_buffer(lpbuffer_t * buf, int channels) {
    size_t i;
    int c, ci;
    lpbuffer_t * newbuf;
    lpfloat_t sample, phase, frac, a, b;

    newbuf = create_buffer(buf->length, channels, buf->samplerate);

    if(channels <= 1) {
        for(i=0; i < buf->length; i++) {
            for(c=0; c < buf->channels; c++) {
                newbuf->data[i] += buf->data[i * buf->channels + c];
            }
        }
    } else {
        for(i=0; i < buf->length; i++) {
            for(c=0; c < channels; c++) {
                phase = (c / (lpfloat_t)(channels-1)) * buf->channels;
                ci = (int)phase;
                frac = phase - ci;
                a = buf->data[i * buf->channels + ci];
                b = buf->data[i * buf->channels + ci+1];
                sample = (1.0f - frac) * a + (frac * b);
                newbuf->data[i * channels + c] = sample;
            }
        }
    }

    return newbuf;
}

lpbuffer_t * resize_buffer(lpbuffer_t * buf, size_t length) {
    size_t i;
    int c;
    lpbuffer_t * newbuf;

    newbuf = create_buffer(length, buf->channels, buf->samplerate);

    for(i=0; i < length; i++) {
        if(i >= buf->length) break;
        for(c=0; c < buf->channels; c++) {
            newbuf->data[i * buf->channels + c] = buf->data[i * buf->channels + c];    
        }
    }

    destroy_buffer(buf);
    return newbuf;
}

void destroy_buffer(lpbuffer_t * buf) {
    if(buf != NULL) {
        LPMemoryPool.free(buf->data);
        LPMemoryPool.free(buf);
    }
}

void destroy_stack(lpstack_t * stack) {
    size_t i;
    if(stack->stack != NULL) {
        for(i=0; i < stack->length; i++) {
            LPBuffer.destroy(stack->stack[i]);
        }
        /*LPMemoryPool.free(stack->stack);*/
    }
    LPMemoryPool.free(stack);
}

/* Basic FX / waveshaping
 */
lpfloat_t read_skewed_buffer(lpfloat_t freq, lpbuffer_t * buf, lpfloat_t phase, lpfloat_t skew) {
    lpfloat_t warp, m, pos;

    m = 0.5f - skew;

    pos = phase / buf->length;
    if(phase < skew) {
        warp = m * (pos / skew);
    } else {
        warp = m * ((1.f-pos) / (1.f-skew));
    }

    return LPInterpolation.linear(buf, (phase + (warp * buf->length)) * freq);
}

lpfloat_t fx_lpf1(lpfloat_t x, lpfloat_t * y, lpfloat_t cutoff, lpfloat_t samplerate) {
    lpfloat_t gamma = 1.f - (lpfloat_t)exp(-(2.f * (lpfloat_t)PI) * (cutoff/samplerate));
    *y = (1.f - gamma) * (*y) + gamma * x;
    return *y;
}

void fx_norm(lpbuffer_t * buf, lpfloat_t ceiling) {
    lpfloat_t maxval, normval;
    size_t i;
    int c;

    maxval = mag_buffer(buf);
    normval = ceiling / maxval;

    for(i=0; i < buf->length; i++) {
        for(c=0; c < buf->channels; c++) {
            buf->data[i * buf->channels + c] *= normval;
        }
    }
}

void fx_convolve(lpbuffer_t * a, lpbuffer_t * b, lpbuffer_t * out) {
    int c;
    size_t i, j;
    lpfloat_t maxval;

    assert(a->channels == b->channels);
    assert(a->channels == out->channels);
    assert(out->length == a->length + b->length + 1);

    maxval = mag_buffer(a);

    for(i=0; i < a->length; i++) {
        for(c=0; c < a->channels; c++) {
            for(j=0; j < b->length; j++) {
                out->data[(j+i) * a->channels + c] += a->data[i * a->channels + c] * b->data[j * a->channels + c];
            }
        }
    }

    fx_norm(out, maxval);
}


/* RingBuffers
 */
lpbuffer_t * ringbuffer_create(size_t length, int channels, int samplerate) {
    lpbuffer_t * ringbuf;
    ringbuf = LPBuffer.create(length, channels, samplerate);
    ringbuf->pos = 0;
    ringbuf->boundry = length - 1;
    return ringbuf;
}

void ringbuffer_fill(lpbuffer_t * ringbuf, lpbuffer_t * buf, int offset) {
    size_t i;
    int c;
    size_t pos = ringbuf->pos - buf->length - offset;
    pos = pos % ringbuf->length;
    for(i=0; i < buf->length; i++) {
        for(c=0; c < ringbuf->channels; c++) {
            buf->data[i * buf->channels + c] = ringbuf->data[pos * ringbuf->channels + c];
        }

        pos += 1;
        pos = pos % ringbuf->length;
    }
}

lpfloat_t ringbuffer_readone(lpbuffer_t * ringbuf, int offset) {
    return ringbuf->data[(ringbuf->pos - offset) % ringbuf->length];
}

void ringbuffer_readinto(lpbuffer_t * ringbuf, lpfloat_t * data, size_t length, int channels) {
    size_t i;
    int c;
    size_t pos = ringbuf->pos - length;
    pos = pos % ringbuf->length;

    for(i=0; i < length; i++) {
        for(c=0; c < channels; c++) {
            data[i * channels + c] = ringbuf->data[pos * channels + c];
        }

        pos += 1;
        pos = pos % ringbuf->length;
    }
}

lpbuffer_t * ringbuffer_read(lpbuffer_t * ringbuf, size_t length) {
    size_t i;
    int c;
    size_t pos = ringbuf->pos - length;
    lpbuffer_t * out;

    pos = pos % ringbuf->length;
    out = LPBuffer.create(length, ringbuf->channels, ringbuf->samplerate);
    for(i=0; i < length; i++) {
        for(c=0; c < ringbuf->channels; c++) {
            out->data[i * out->channels + c] = ringbuf->data[pos * ringbuf->channels + c];
        }

        pos += 1;
        pos = pos % ringbuf->length;
    }

    return out;
}

void ringbuffer_writeone(lpbuffer_t * ringbuf, lpfloat_t sample) {
    ringbuf->data[ringbuf->pos] = sample;
    ringbuf->pos += 1;
    ringbuf->pos = ringbuf->pos % ringbuf->length;
}

void ringbuffer_writefrom(lpbuffer_t * ringbuf, lpfloat_t * data, size_t length, int channels) {
    size_t i;
    int c, j;
    for(i=0; i < length; i++) {
        for(c=0; c < ringbuf->channels; c++) {
            j = c % channels;
            ringbuf->data[ringbuf->pos * ringbuf->channels + c] = data[i * channels + j];
        }

        ringbuf->pos += 1;
        ringbuf->pos = ringbuf->pos % ringbuf->length;
    }
}

void ringbuffer_write(lpbuffer_t * ringbuf, lpbuffer_t * buf) {
    size_t i;
    int c, j;
    for(i=0; i < buf->length; i++) {
        for(c=0; c < ringbuf->channels; c++) {
            j = c % buf->channels;
            ringbuf->data[ringbuf->pos * ringbuf->channels + c] = buf->data[i * buf->channels + j];
        }

        ringbuf->pos += 1;
        ringbuf->pos = ringbuf->pos % ringbuf->length;
    }
}

void ringbuffer_dub(lpbuffer_t * buf, lpbuffer_t * src) {
    size_t i;
    int c, j;
    for(i=0; i < src->length; i++) {
        for(c=0; c < buf->channels; c++) {
            j = c % src->channels;
            buf->data[buf->pos * buf->channels + c] += src->data[i * src->channels + j];
        }

        buf->pos += 1;
        buf->pos = buf->pos % buf->length;
    }
}

void ringbuffer_destroy(lpbuffer_t * buf) {
    LPBuffer.destroy(buf);
}


/* LPMemoryPool
 * */
void memorypool_init(unsigned char * pool, size_t poolsize) {
    assert(poolsize >= 1);
    LPMemoryPool.pool = pool;
    LPMemoryPool.poolsize = poolsize;
    LPMemoryPool.pos = 0;
}

lpmemorypool_t * memorypool_custom_init(unsigned char * pool, size_t poolsize) {
    lpmemorypool_t * mp;
    mp = (lpmemorypool_t *)LPMemoryPool.alloc(1, sizeof(lpmemorypool_t));

    assert(poolsize >= 1);
    mp->pool = pool;
    mp->poolsize = poolsize;
    mp->pos = 0;

    return mp;
}

void * memorypool_custom_alloc(lpmemorypool_t * mp, size_t itemcount, size_t itemsize) {
    void * p;
    size_t length;

    assert(mp->pool != 0); 
    length = itemcount * itemsize;

    if(mp->poolsize >= mp->pos + length) {
        p = (void *)(&mp->pool[mp->pos]);
        mp->pos += length;
        return p;
    }
    /* FIXME might as well try to expand the pool here */
    exit(EXIT_FAILURE);
}

void * memorypool_alloc(size_t itemcount, size_t itemsize) {
#ifdef LP_STATIC
    void * p;
    size_t length;

    assert(LPMemoryPool.pool != 0); 
    length = itemcount * itemsize;

    if(LPMemoryPool.poolsize >= LPMemoryPool.pos + length) {
        p = (void *)(&LPMemoryPool.pool[LPMemoryPool.pos]);
        LPMemoryPool.pos += length;
        return p;
    }
    exit(EXIT_FAILURE);
#else
    return calloc(itemcount, itemsize);
#endif
}

void memorypool_free(void * ptr) {
#ifndef LP_STATIC
    if(ptr != NULL) free(ptr);
#else
    (void)ptr;
#endif
}

/* Param
 * */
lpbuffer_t * param_create_from_float(lpfloat_t value) {
    lpbuffer_t * param = create_buffer(1, 1, DEFAULT_SAMPLERATE);
    param->data[0] = value;
    return param;
}

lpbuffer_t * param_create_from_int(int value) {
    lpbuffer_t * param = create_buffer(1, 1, DEFAULT_SAMPLERATE);
    param->data[0] = (lpfloat_t)value;
    return param;
}

/* Interpolation
 * */
lpfloat_t interpolate_hermite(lpbuffer_t* buf, lpfloat_t phase) {
    lpfloat_t y0, y1, y2, y3, frac;
    lpfloat_t c0, c1, c2, c3;
    int i0, i1, i2, i3;

    if(buf->range == 1) return buf->data[0];
    if(buf->range < 1) return 0;

    frac = phase - (int)phase;
    i1 = (int)phase;
    i2 = i1 + 1;
    i3 = i2 + 1;
    i0 = i1 - 1;

    y0 = 0;
    y1 = 0;
    y2 = 0;
    y3 = 0;

    if(i0 >= 0) y0 = buf->data[i0];
    if(i1 <= (int)buf->boundry) y1 = buf->data[i1];
    if(i2 <= (int)buf->boundry) y2 = buf->data[i2];
    if(i3 <= (int)buf->boundry) y3 = buf->data[i3];

    /* This part was taken from version #2 by James McCartney 
     * https://www.musicdsp.org/en/latest/Other/93-hermite-interpollation.html
     */
    c0 = y1;
    c1 = 0.5f * (y2 - y0);
    c3 = 1.5f * (y1 - y2) + 0.5f * (y3 - y0);
    c2 = y0 - y1 + c1 - c3;
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

lpfloat_t interpolate_hermite_pos(lpbuffer_t* buf, lpfloat_t pos) {
    return interpolate_hermite(buf, pos * buf->length);
}

/* Interpolated read from a multichannel buffer
 */
lpfloat_t interpolate_linear_channel(lpbuffer_t* buf, lpfloat_t phase, int channel) {
    lpfloat_t frac, a, b;
    size_t i;

    if(buf->range == 1) return buf->data[0];
    
    frac = phase - (int)phase;
    i = (int)phase;

    if (i >= buf->boundry) return 0;

    a = buf->data[i * buf->channels + channel];
    b = buf->data[(i+1) * buf->channels + channel];

    return (1.0f - frac) * a + (frac * b);
}

/* Interpolated read from a single channel buffer
 */
lpfloat_t interpolate_linear(lpbuffer_t* buf, lpfloat_t phase) {
    lpfloat_t frac, a, b;
    size_t i;

    if(buf->range == 1) return buf->data[0];
    
    frac = phase - (int)phase;
    i = (int)phase;

    if (i >= buf->boundry) return 0;

    a = buf->data[i];
    b = buf->data[i+1];

    return (1.0f - frac) * a + (frac * b);
}

lpfloat_t interpolate_linear_pos(lpbuffer_t* buf, lpfloat_t pos) {
    return interpolate_linear(buf, pos * buf->length);
}

/* Wavetable generators
 * 
 * All these functions return a table of values 
 * of the given length with values between -1 and 1
 */
void wavetable_sine(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = sin((i/(lpfloat_t)length) * (lpfloat_t)PI * 2.0f);
    }
}

void wavetable_cosine(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = cos((i/(lpfloat_t)length) * (lpfloat_t)PI * 2.0f);
    }
}

void wavetable_square(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        if(i < (length/2.0f)) {
            out[i] = 0.9999f;
        } else {
            out[i] = -0.9999f;
        }
    }
}

void wavetable_tri(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = (lpfloat_t)fabs((i/(lpfloat_t)length) * 2.0f - 1.0f) * 2.0f - 1.0f;
    }
}

void wavetable_saw(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = (i/(lpfloat_t)length) * 2.f - 1.f;
    }
}

void wavetable_rsaw(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = (1.f - i/(lpfloat_t)length) * 2.f - 1.f;
    }
}

/* create a wavetable (-1 to 1) */
lpbuffer_t* create_wavetable(int name, size_t length) {
    lpbuffer_t* buf = LPBuffer.create(length, 1, -1);
    if(name == WT_SINE) {
        wavetable_sine(buf->data, length);            
    } else if (name == WT_COS) {
        wavetable_cosine(buf->data, length);            
    } else if (name == WT_TRI) {
        wavetable_tri(buf->data, length);            
    } else if (name == WT_SQUARE) {
        wavetable_square(buf->data, length);            
    } else if (name == WT_SAW) {
        wavetable_saw(buf->data, length);            
    } else if (name == WT_RSAW) {
        wavetable_rsaw(buf->data, length);            
    } else if (name == WT_RND) {
        return create_wavetable(rand_choice(NUM_WAVETABLES), length);
    } else {
        wavetable_sine(buf->data, length);            
    }
    return buf;
}

lpstack_t * create_wavetable_stack(int numtables, ...) {
    va_list vl;
    lpstack_t * stack;
    int i, name;

    va_start(vl, numtables);

    stack = (lpstack_t *)LPMemoryPool.alloc(1, sizeof(lpstack_t));
    stack->stack = (lpbuffer_t **)LPMemoryPool.alloc(numtables, sizeof(lpbuffer_t *));
    stack->length = numtables;
    stack->phase = 0.f;
    stack->pos = 0.f;

    for(i=0; i < numtables; i++) {
        name = va_arg(vl, int);
        stack->stack[i] = LPWavetable.create(name, DEFAULT_TABLESIZE); 
    }

    va_end(vl);

    return stack;
}

lpstack_t * create_uniform_stack(int numbuffers, size_t buffer_length, int channels, int samplerate) {
    lpstack_t * stack;
    int i;

    stack = (lpstack_t *)LPMemoryPool.alloc(1, sizeof(lpstack_t));
    stack->stack = (lpbuffer_t **)LPMemoryPool.alloc(numbuffers, sizeof(lpbuffer_t *));
    stack->length = numbuffers;
    stack->phase = 0.f;
    stack->pos = 0.f;
    stack->read_index = 0;

    for(i=0; i < numbuffers; i++) {
        stack->stack[i] = LPBuffer.create(buffer_length, channels, samplerate); 
    }

    return stack;
}


void destroy_wavetable(lpbuffer_t* buf) {
    LPBuffer.destroy(buf);
}


/* Window generators
 *
 * All these functions return a table of values 
 * of the given length with values between 0 and 1
 */
void window_phasor(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = i/(lpfloat_t)length;      
    }
}

void window_rsaw(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = 1.f - (i/(lpfloat_t)length);
    }
}

void window_tri(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = fabs((i/(lpfloat_t)length) * 2.0f - 1.0f);
    }
}

void window_cosine(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = (lpfloat_t)cos((i/(lpfloat_t)length) * (lpfloat_t)PI);         
    }
}

void window_sine(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = (lpfloat_t)sin((i/(lpfloat_t)length) * (lpfloat_t)PI);         
    }
}

void window_sinein(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = (lpfloat_t)sin((i/(lpfloat_t)length) * (lpfloat_t)HALFPI);         
    }
}

void window_sineout(lpfloat_t* out, int length) {
    int i;
    for(i=0; i < length; i++) {
        out[i] = (lpfloat_t)sin(((i/(lpfloat_t)length) * (lpfloat_t)HALFPI) + (lpfloat_t)HALFPI);
    }
}

void window_hanning(lpfloat_t* out, int length) {
    int i;
    assert(length > 1);
    for(i=0; i < length; i++) {
        out[i] = 0.5f - 0.5f * (lpfloat_t)cos(2.0f * (lpfloat_t)PI * i / (length-1.0f));
    }
}

/* create a window (0 to 1) */
lpbuffer_t * create_window(int name, size_t length) {
    lpbuffer_t* buf = LPBuffer.create(length, 1, -1);
    if(name == WIN_SINE) {
        window_sine(buf->data, length);            
    } else if (name == WIN_SINEIN) {
        window_sinein(buf->data, length);            
    } else if (name == WIN_SINEOUT) {
        window_sineout(buf->data, length);            
    } else if (name == WIN_COS) {
        window_cosine(buf->data, length);            
    } else if (name == WIN_TRI) {
        window_tri(buf->data, length);            
    } else if (name == WIN_PHASOR) {
        window_phasor(buf->data, length);            
    } else if (name == WIN_HANN) {
        window_hanning(buf->data, length);            
    } else if (name == WIN_SAW) {
        window_phasor(buf->data, length);            
    } else if (name == WIN_RSAW) {
        window_rsaw(buf->data, length);            
    } else if (name == WIN_RND) {
        return create_window(rand_choice(NUM_WINDOWS), length);
    } else {
        window_sine(buf->data, length);            
    }
    return buf;
}

lpstack_t * create_window_stack(int numtables, ...) {
    va_list vl;
    lpstack_t * stack;
    int i, name;

    va_start(vl, numtables);

    stack = (lpstack_t *)LPMemoryPool.alloc(1, sizeof(lpstack_t));
    stack->stack = (lpbuffer_t **)LPMemoryPool.alloc(numtables, sizeof(lpbuffer_t *));
    stack->length = numtables;
    stack->phase = 0.f;
    stack->pos = 0.f;

    for(i=0; i < numtables; i++) {
        name = va_arg(vl, int);
        stack->stack[i] = LPWindow.create(name, DEFAULT_TABLESIZE); 
    }

    va_end(vl);

    return stack;
}

void destroy_window(lpbuffer_t* buf) {
    LPBuffer.destroy(buf);
}

/* Utilities */

/* The zapgremlins() routine was written by James McCartney as part of SuperCollider:
 * https://github.com/supercollider/supercollider/blob/f0d4f47a33b57b1f855fe9ca2d4cb427038974f0/headers/plugin_interface/SC_InlineUnaryOp.h#L35
 *
 * SuperCollider real time audio synthesis system
 * Copyright (c) 2002 James McCartney. All rights reserved.
 * http://www.audiosynth.com
 *
 * He says:
 *      This is a function for preventing pathological math operations in ugens.
 *      It can be used at the end of a block to fix any recirculating filter values.
 */
lpfloat_t lpzapgremlins(lpfloat_t x) {
    lpfloat_t absx;
    absx = fabs(x);
    return (absx > (lpfloat_t)1e-15 && absx < (lpfloat_t)1e15) ? x : (lpfloat_t)0.f;
}

lpfloat_t lpwv(lpfloat_t value, lpfloat_t min, lpfloat_t max) {
    /* wrap value */
    if(value > max) value -= max;
    if(value < min) value += min;
    return value;
}

lpfloat_t lpsv(lpfloat_t value, lpfloat_t min, lpfloat_t max) {
    /* scale value */
    return value * (max-min) + min;
}

lpfloat_t lpsvf(lpfloat_t value, lpfloat_t min, lpfloat_t max, lpfloat_t from, lpfloat_t to) {
    /* scale value from (a range other than 0-1) */
    lpfloat_t delta = to - from;
    if(delta <= 0) return 0;
    return (value/delta) * (max-min) + min;
}

lpfloat_t lpfmax(lpfloat_t a, lpfloat_t b) {
    if(a > b) return a;
    return b;
}

lpfloat_t lpfmin(lpfloat_t a, lpfloat_t b) {
    if(a < b) return a;
    return b;
}

lpfloat_t lpfabs(lpfloat_t value) {
    if(value <= 0) return value * -1;
    return value;
}

