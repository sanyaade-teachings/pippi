#include "astrid.h"

#define NAME "pulsar"

#define SR 48000
#define CHANNELS 2
#define ADC_LENGTH 30

#define NUMOSCS (CHANNELS * 10)
#define WTSIZE 4096
#define NUMFREQS 12

lpfloat_t scale[] = {
    55.000f, 
    110.000f, 
    122.222f, 
    137.500f, 
    146.667f, 
    165.000f, 
    185.625f, 
    206.250f, 
    220.000f, 
    244.444f, 
    275.000f, 
    293.333f
};

enum InstrumentParams {
    PARAM_FREQ,
    PARAM_FREQS,
    PARAM_AMP,
    PARAM_PW,
    NUMPARAMS
};

typedef struct localctx_t {
    lppulsarosc_t * oscs[NUMOSCS];
    lpbuffer_t * env;
    int last;

    lpbuffer_t * curves[NUMOSCS];
    lpfloat_t env_phases[NUMOSCS];
    lpfloat_t env_phaseincs[NUMOSCS];
    lpfloat_t selected_freqs[NUMFREQS];
} localctx_t;

int param_update_callback(void * arg) {
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    localctx_t * ctx = (localctx_t *)instrument->context;

    // Respond to param update messages -- TODO: parse param args with PARAM_ consts
    syslog(LOG_DEBUG, "MSG: update | %s\n", instrument->msg.msg);
    for(int i=0; i < NUMFREQS; i++) {
        ctx->selected_freqs[i] = scale[LPRand.randint(0, NUMFREQS*2) % NUMFREQS] * 0.5f + LPRand.rand(0.f, 1.f);
    }
    astrid_instrument_set_param_float_list(instrument, PARAM_FREQS, ctx->selected_freqs, NUMFREQS);
    astrid_instrument_set_param_float(instrument, PARAM_AMP, LPRand.rand(0.5f, 1.f));
    astrid_instrument_set_param_float(instrument, PARAM_PW, LPRand.rand(0.05f, 1.f));
    return 0;
}

int renderer_callback(void * arg) {
    lpbuffer_t * out;
    lpinstrument_t * instrument = (lpinstrument_t *)arg;

    out = LPBuffer.create(LPRand.randint(0, SR), instrument->channels, SR);
    if(lpsampler_read_ringbuffer_block(instrument->adcname, instrument->adcbuf, 0, out) < 0) {
        return -1;
    }
    LPFX.norm(out, LPRand.rand(0.26f, 0.5f));

    if(send_render_to_mixer(instrument, out) < 0) {
        return -1;
    }

    return 0;
}

int audio_callback(size_t blocksize, __attribute__((unused)) float ** input, float ** output, void * arg) {
    size_t i;
    int j;
    lpfloat_t freqs[NUMFREQS];
    lpfloat_t sample, left, right, amp, pw, saturation, pan;
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    localctx_t * ctx = (localctx_t *)instrument->context;

    if(!instrument->is_running) return 1;

    amp = astrid_instrument_get_param_float(instrument, PARAM_AMP, 0.08f);
    pw = astrid_instrument_get_param_float(instrument, PARAM_PW, 1.f);
    astrid_instrument_get_param_float_list(instrument, PARAM_FREQS, NUMFREQS, freqs);

    for(i=0; i < blocksize; i++) {
        sample = 0.f;
        for(j=0; j < NUMOSCS; j++) {
            saturation = LPInterpolation.linear_pos(ctx->curves[j], ctx->curves[j]->phase) * 0.05f + 0.95f;
            pan = LPInterpolation.linear_pos(ctx->curves[j], ctx->curves[j]->phase);
            ctx->oscs[j]->saturation = saturation;
            ctx->oscs[j]->pulsewidth = pw;
            ctx->oscs[j]->freq = freqs[j % NUMFREQS] * 4.f * 0.6f;

            sample += LPPulsarOsc.process(ctx->oscs[j]) * amp * LPInterpolation.linear_pos(ctx->env, ctx->env_phases[j]) * 0.12f;

            ctx->env_phases[j] += ctx->env_phaseincs[j];

            if(ctx->env_phases[j] >= 1.f) {
                // change the pitch when the env boundry is crossed
                ctx->selected_freqs[ctx->last] = scale[LPRand.randint(0, NUMFREQS*2) % NUMFREQS] * 0.5f + LPRand.rand(0.f, 1.f);
                ctx->last += 1;
                while(ctx->last >= NUMFREQS) ctx->last -= NUMFREQS;
            }

            while(ctx->env_phases[j] >= 1.f) ctx->env_phases[j] -= 1.f;

            ctx->curves[j]->phase += ctx->env_phaseincs[j];
            while(ctx->curves[j]->phase >= 1.f) ctx->curves[j]->phase -= 1.f;
        }

        left = right = sample;
        pan_stereo_constant(pan, left, right, &left, &right);

        output[0][i] += left;
        output[1][i] += right;
    }

    return 0;
}

int main() {
    lpinstrument_t * instrument;
    
    // create local context struct
    localctx_t * ctx = (localctx_t *)calloc(1, sizeof(localctx_t));
    if(ctx == NULL) {
        printf("Could not alloc ctx: (%d) %s\n", errno, strerror(errno));
        exit(1);
    }

    // create env window for the footballs
    ctx->env = LPWindow.create(WIN_HANNOUT, 4096);

    // setup oscs and curves
    for(int i=0; i < NUMOSCS; i++) {
        ctx->env_phases[i] = LPRand.rand(0.f, 1.f);
        ctx->env_phaseincs[i] = (1.f/SR) * LPRand.rand(0.005f, 0.03f);
        ctx->curves[i] = LPWindow.create(WIN_RND, 4096);

        ctx->oscs[i] = LPPulsarOsc.create(2, 2, // number of wavetables, windows
            WT_SINE, WTSIZE, WT_TRI2, WTSIZE,   // wavetables and sizes
            WIN_SINE, WTSIZE, WIN_HANN, WTSIZE  // windows and sizes
        );

        ctx->oscs[i]->samplerate = (lpfloat_t)SR;
        ctx->oscs[i]->wavetable_morph_freq = LPRand.rand(0.001f, 0.15f);
        ctx->oscs[i]->phase = 0.f;
    }

    // Set the callbacks for streaming, async renders and param updates
    if((instrument = astrid_instrument_start(NAME, CHANNELS, 0, ADC_LENGTH, (void*)ctx, 
                    audio_callback, renderer_callback, param_update_callback, NULL)) == NULL) {
        fprintf(stderr, "Could not start instrument: (%d) %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* now that LMDB is running, populate the initial freqs */
    for(int i=0; i < NUMFREQS; i++) {
        ctx->selected_freqs[i] = scale[LPRand.randint(0, NUMFREQS*2) % NUMFREQS] * 0.5f + LPRand.rand(0.f, 1.f);
    }
    astrid_instrument_set_param_float_list(instrument, PARAM_FREQS, ctx->selected_freqs, NUMFREQS);

    /* twiddle thumbs until shutdown */
    while(instrument->is_running) {
        astrid_instrument_tick(instrument);
    }

    /* stop jack and cleanup threads */
    if(astrid_instrument_stop(instrument) < 0) {
        fprintf(stderr, "There was a problem stopping the instrument. (%d) %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* clean up local memory */
    for(int o=0; o < NUMOSCS; o++) {
        LPPulsarOsc.destroy(ctx->oscs[o]);
        LPBuffer.destroy(ctx->curves[o]);
    }

    LPBuffer.destroy(ctx->env);

    free(ctx);

    printf("Done!\n");
    return 0;
}
