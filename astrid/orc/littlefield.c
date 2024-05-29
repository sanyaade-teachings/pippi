#include "astrid.h"

#define NAME "littlefield"

#define SR 48000
#define CHANNELS 2
#define ADC_LENGTH 30
#define MIC_ATTENUATION 0.2f

#define NUMOSCS 20
#define WTSIZE 4096
#define NUMFREQS 12

lppatternbuf_t default_patternbuf = {1,{1}};

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
    PARAM_NOOP,
    PARAM_FREQ,
    PARAM_FREQS,
    PARAM_AMP,
    PARAM_WINSPEED,
    PARAM_WINFREQ,
    PARAM_OSC_MIX,
    PARAM_MIC_MIX,
    PARAM_ENV_MOD,
    PARAM_OCTAVE,
    PARAM_GATE_PATTERN,
    PARAM_PW,
    NUMPARAMS
};

typedef struct localctx_t {
    lppulsarosc_t * oscs[NUMOSCS];
    lppulsarosc_t * gate;
    lpbuffer_t * env;
    int last;

    lpbuffer_t * curves[NUMOSCS];
    lpfloat_t env_phases[NUMOSCS];
    lpfloat_t env_phaseincs[NUMOSCS];
    lpfloat_t selected_freqs[NUMFREQS];
} localctx_t;

lpfloat_t osc_mix(lpfloat_t pos, int chan) {
    if(chan == 0) {
        if(pos < 0.5f) return 1.f;
        return 1.f - ((pos - 0.5f) * 2.f);
    } else {
        if(pos < 0.5f) return pos * 2.f;
        return 1.f;
    }
}

lpfloat_t mic_mix(lpfloat_t pos, int chan) {
    if(chan == 0) {
        if(pos < 0.5f) return 1.f - (pos * 2.f);
        return 0;
    } else {
        if(pos < 0.5f) return 0.f;
        return (pos-0.5f) * 2.f;
    }
}

int param_map_callback(void * arg, char * keystr, char * valstr) {
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    localctx_t * ctx = (localctx_t *)instrument->context;
    float val_f = 0;
    uint32_t val_i32 = 0;
    lppatternbuf_t val_pattern = {1,{1}};

    syslog(LOG_DEBUG, "Got param %s=%s\n", keystr, valstr);

    if(strcmp(keystr, "amp") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_AMP, val_f);
    } else if(strcmp(keystr, "pw") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_PW, val_f);
    } else if(strcmp(keystr, "ws") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_WINSPEED, val_f);
    } else if(strcmp(keystr, "wf") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_WINFREQ, val_f);
    } else if(strcmp(keystr, "omix") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_MIX, val_f);
    } else if(strcmp(keystr, "mmix") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_MIC_MIX, val_f);
    } else if(strcmp(keystr, "gpat") == 0) {
        extract_patternbuf_from_token(valstr, val_pattern.pattern, &val_pattern.length);
        astrid_instrument_set_param_patternbuf(instrument, PARAM_GATE_PATTERN, &val_pattern);
    } else if(strcmp(keystr, "emod") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_ENV_MOD, val_f);
    } else if(strcmp(keystr, "freqs") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        ctx->selected_freqs[LPRand.randint(0, NUMFREQS)] = scale[val_i32 % NUMFREQS] * 0.5f + LPRand.rand(0.f, 1.f);
        astrid_instrument_set_param_float_list(instrument, PARAM_FREQS, ctx->selected_freqs, NUMFREQS);
    } else if (strcmp(keystr, "oct") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        astrid_instrument_set_param_float(instrument, PARAM_OCTAVE, val_i32);
    }
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

int audio_callback(size_t blocksize, float ** input, float ** output, void * arg) {
    size_t i;
    int j;
    lpfloat_t freqs[NUMFREQS];
    lpfloat_t sample, left, right, amp, env, pw, saturation, 
              winspeed, winfreq,
              osc_mix_ctl, mic_mix_ctl, in0, in1, env_mod;
    lppatternbuf_t gate_pattern = {1,{1}};
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    localctx_t * ctx = (localctx_t *)instrument->context;

    if(!instrument->is_running) return 1;

    amp = astrid_instrument_get_param_float(instrument, PARAM_AMP, 0.5f);
    env_mod = astrid_instrument_get_param_float(instrument, PARAM_ENV_MOD, 0.999f);
    osc_mix_ctl = astrid_instrument_get_param_float(instrument, PARAM_OSC_MIX, 0.5f);
    mic_mix_ctl = astrid_instrument_get_param_float(instrument, PARAM_MIC_MIX, 0.25f);
    winspeed = astrid_instrument_get_param_float(instrument, PARAM_WINSPEED, 0.5f) * 1000.f + 1.f;
    winfreq = astrid_instrument_get_param_float(instrument, PARAM_WINFREQ, 1.f);
    pw = astrid_instrument_get_param_float(instrument, PARAM_PW, 1.f);
    astrid_instrument_get_param_float_list(instrument, PARAM_FREQS, NUMFREQS, freqs);
    gate_pattern = astrid_instrument_get_param_patternbuf(instrument, PARAM_GATE_PATTERN, default_patternbuf);

    ctx->gate->window_morph = env_mod;
    ctx->gate->freq = winfreq;
    LPPulsarOsc.burst_bytes(ctx->gate, gate_pattern.pattern, gate_pattern.length);

    for(i=0; i < blocksize; i++) {
        sample = 0.f;
        for(j=0; j < NUMOSCS; j++) {
            saturation = LPInterpolation.linear_pos(ctx->curves[j], ctx->curves[j]->phase) * 0.05f + 0.95f;
            ctx->oscs[j]->saturation = saturation;
            ctx->oscs[j]->pulsewidth = pw * input[0][i];
            ctx->oscs[j]->freq = freqs[j % NUMFREQS] * 4.f;

            sample += LPPulsarOsc.process(ctx->oscs[j]) * LPInterpolation.linear_pos(ctx->env, ctx->env_phases[j]) * ((1.f/NUMOSCS)*3.f);

            ctx->env_phases[j] += ctx->env_phaseincs[j] * (1.f+winspeed) * 3.f;

            if(ctx->env_phases[j] >= 1.f) {
                // change the pitch when the env boundry is crossed
                ctx->selected_freqs[ctx->last] = scale[LPRand.randint(0, NUMFREQS*2) % NUMFREQS] * (1 << LPRand.randint(0, 4)) + LPRand.rand(0.f, 1.f);
                ctx->last += 1;
                while(ctx->last >= NUMFREQS) ctx->last -= NUMFREQS;
            }

            while(ctx->env_phases[j] >= 1.f) ctx->env_phases[j] -= 1.f;

            ctx->curves[j]->phase += ctx->env_phaseincs[j];
            while(ctx->curves[j]->phase >= 1.f) ctx->curves[j]->phase -= 1.f;
        }

        left = right = sample * amp;

        left *= osc_mix(osc_mix_ctl, 0);
        right *= osc_mix(osc_mix_ctl, 1);
        in0 = input[0][i] * mic_mix(mic_mix_ctl, 0) * MIC_ATTENUATION;
        in1 = input[1][i] * mic_mix(mic_mix_ctl, 1) * MIC_ATTENUATION;

        env = LPPulsarOsc.process(ctx->gate);
        output[0][i] += (left + in0) * env;
        output[1][i] += (right + in1) * env;
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
        ctx->env_phaseincs[i] = (1.f/SR) * LPRand.rand(0.005f, 0.03f) * 0.5f;
        ctx->curves[i] = LPWindow.create(WIN_RND, 4096);

        ctx->oscs[i] = LPPulsarOsc.create(2, 2, // number of wavetables, windows
            WT_SINE, WTSIZE, WT_TRI2, WTSIZE,   // wavetables and sizes
            WIN_SINE, WTSIZE, WIN_HANN, WTSIZE  // windows and sizes
        );

        ctx->oscs[i]->samplerate = (lpfloat_t)SR;
        ctx->oscs[i]->wavetable_morph_freq = LPRand.rand(0.001f, 0.15f);
        ctx->oscs[i]->phase = 0.f;
    }

    ctx->gate = LPPulsarOsc.create(0, 4,
        WIN_SINEOUT, WTSIZE, 
        WIN_SINE, WTSIZE, 
        WIN_PHASOR, WTSIZE, 
        WIN_SINEOUT, WTSIZE
    );

    ctx->gate->samplerate = (lpfloat_t)SR;
    ctx->gate->window_morph_freq = 0;
    ctx->gate->freq = 30.f;
    ctx->gate->phase = 0.f;

    // Set the callbacks for streaming, async renders and param updates
    if((instrument = astrid_instrument_start(NAME, CHANNELS, 0, ADC_LENGTH, (void*)ctx, 
                    audio_callback, renderer_callback, param_map_callback, NULL)) == NULL) {
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

    LPPulsarOsc.destroy(ctx->gate);
    LPBuffer.destroy(ctx->env);

    free(ctx);

    printf("Done!\n");
    return 0;
}
