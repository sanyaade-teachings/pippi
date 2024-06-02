#include "astrid.h"

#define NAME "littlefield"

#define SR 48000
#define CHANNELS 2
#define ADC_LENGTH 30
#define MIC_ATTENUATION 0.2f

#define NUMOSCS 20
#define WTSIZE 4096
#define MAXNUMFREQS NUMOSCS

int num_freqs = MAXNUMFREQS;

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
    PARAM_OSC_AMP,
    PARAM_OSC_MIX,
    PARAM_OSC_PULSEWIDTH,
    PARAM_OSC_SATURATION,
    PARAM_OSC_ENVELOPE_SPEED,
    PARAM_OSC_DRIFT_DEPTH,
    PARAM_OSC_DISTORTION_AMOUNT,
    PARAM_OSC_OCTAVE_SPREAD,
    PARAM_OSC_OCTAVE_OFFSET,
    PARAM_OSC_FREQS,
    PARAM_GATE_AMP,
    PARAM_GATE_MIX,
    PARAM_GATE_SPEED,
    PARAM_GATE_PULSEWIDTH,
    PARAM_GATE_SATURATION,
    PARAM_GATE_SHAPE,
    PARAM_GATE_DRIFT_DEPTH,
    PARAM_GATE_PATTERN,
    PARAM_GATE_PHASE_RESET,
    PARAM_MIC_MIX,
    NUMPARAMS
};

typedef struct localctx_t {
    lppulsarosc_t * oscs[NUMOSCS];
    lppulsarosc_t * gate;
    lpfloat_t foldprev;
    lpfloat_t limitprev;
    lpbuffer_t * limitbuf;
    lpbuffer_t * env;
    lpbuffer_t * curves[NUMOSCS];
    lpfloat_t env_phases[NUMOSCS];
    lpfloat_t env_phaseincs[NUMOSCS];
    lpfloat_t selected_freqs[MAXNUMFREQS];
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
    //localctx_t * ctx = (localctx_t *)instrument->context;
    lpfloat_t val_floatlist[MAXNUMFREQS] = {220.};
    float val_f = 0;
    uint32_t val_i32 = 0;
    lppatternbuf_t val_pattern = {1,{1}};

    syslog(LOG_DEBUG, "Got param %s=%s\n", keystr, valstr);

    if(strcmp(keystr, "oamp") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_AMP, val_f);
    } else if(strcmp(keystr, "omix") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_MIX, val_f);
    } else if(strcmp(keystr, "opw") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_PULSEWIDTH, val_f);
    } else if(strcmp(keystr, "osat") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_SATURATION, val_f);
    } else if(strcmp(keystr, "ospeed") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_ENVELOPE_SPEED, val_f);
    } else if(strcmp(keystr, "odrift") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_DRIFT_DEPTH, val_f);
    } else if(strcmp(keystr, "odist") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_DISTORTION_AMOUNT, val_f);
    } else if(strcmp(keystr, "octspread") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        astrid_instrument_set_param_int32(instrument, PARAM_OSC_OCTAVE_SPREAD, val_i32);
    } else if(strcmp(keystr, "octoffset") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        astrid_instrument_set_param_int32(instrument, PARAM_OSC_OCTAVE_OFFSET, val_i32);
    } else if(strcmp(keystr, "freqs") == 0) {
        num_freqs = extract_floatlist_from_token(valstr, val_floatlist, MAXNUMFREQS);
        astrid_instrument_set_param_float_list(instrument, PARAM_OSC_FREQS, val_floatlist, num_freqs);
    } else if(strcmp(keystr, "gamp") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_AMP, val_f);
    } else if(strcmp(keystr, "gmix") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_MIX, val_f);
    } else if(strcmp(keystr, "gspeed") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_SPEED, val_f);
    } else if(strcmp(keystr, "gpw") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_PULSEWIDTH, val_f);
    } else if(strcmp(keystr, "gsat") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_SATURATION, val_f);
    } else if(strcmp(keystr, "gshape") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_SHAPE, val_f);
    } else if(strcmp(keystr, "gdrift") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_DRIFT_DEPTH, val_f);
    } else if(strcmp(keystr, "gpat") == 0) {
        extract_patternbuf_from_token(valstr, val_pattern.pattern, &val_pattern.length);
        astrid_instrument_set_param_patternbuf(instrument, PARAM_GATE_PATTERN, &val_pattern);
    } else if(strcmp(keystr, "greset") == 0) {
        astrid_instrument_set_param_int32(instrument, PARAM_GATE_PHASE_RESET, 1);
    } else if(strcmp(keystr, "mmix") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_MIC_MIX, val_f);
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
    lpfloat_t freqs[MAXNUMFREQS];
    lpfloat_t sample, left, right, amp, env, in0, in1;
    lpfloat_t osc_pw, osc_saturation, osc_env_speed, 
              osc_distortion, osc_mix_ctl, osc_drift, 
              mic_mix_ctl;
    uint32_t octave_spread, octave_offset, gate_reset;
    lpfloat_t gate_shape, gate_speed, gate_pw, 
              gate_saturation;
    lppatternbuf_t gate_pattern = {1,{1}};
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    localctx_t * ctx = (localctx_t *)instrument->context;

    if(!instrument->is_running) return 1;

    amp = astrid_instrument_get_param_float(instrument, PARAM_OSC_AMP, 0.5f);
    osc_mix_ctl = astrid_instrument_get_param_float(instrument, PARAM_OSC_MIX, 0.5f);
    osc_pw = astrid_instrument_get_param_float(instrument, PARAM_OSC_PULSEWIDTH, 1.f);
    osc_saturation = astrid_instrument_get_param_float(instrument, PARAM_OSC_SATURATION, 1.f);
    osc_env_speed = astrid_instrument_get_param_float(instrument, PARAM_OSC_ENVELOPE_SPEED, 0.5f) * 1000.f + 1.f;
    osc_drift = astrid_instrument_get_param_float(instrument, PARAM_OSC_DRIFT_DEPTH, 0.f);
    osc_distortion = astrid_instrument_get_param_float(instrument, PARAM_OSC_DISTORTION_AMOUNT, 0.f);
    octave_spread = astrid_instrument_get_param_int32(instrument, PARAM_OSC_OCTAVE_SPREAD, 0);
    octave_offset = astrid_instrument_get_param_int32(instrument, PARAM_OSC_OCTAVE_OFFSET, 2);
    astrid_instrument_get_param_float_list(instrument, PARAM_OSC_FREQS, num_freqs, freqs);

    gate_reset = astrid_instrument_get_param_int32(instrument, PARAM_GATE_PHASE_RESET, 0);
    gate_shape = astrid_instrument_get_param_float(instrument, PARAM_GATE_SHAPE, 0.f);
    gate_speed = astrid_instrument_get_param_float(instrument, PARAM_GATE_SPEED, 1.f);
    gate_saturation = astrid_instrument_get_param_float(instrument, PARAM_GATE_SATURATION, 1.f);
    gate_pw = astrid_instrument_get_param_float(instrument, PARAM_GATE_PULSEWIDTH, 1.f);
    gate_pattern = astrid_instrument_get_param_patternbuf(instrument, PARAM_GATE_PATTERN, default_patternbuf);

    mic_mix_ctl = astrid_instrument_get_param_float(instrument, PARAM_MIC_MIX, 0.25f);


    ctx->gate->window_morph = fmin(gate_shape, 0.999f); // TODO investigate overflows
    ctx->gate->freq = gate_speed;
    ctx->gate->pulsewidth = gate_pw;
    ctx->gate->saturation = gate_saturation;
    LPPulsarOsc.burst_bytes(ctx->gate, gate_pattern.pattern, gate_pattern.length);

    if(gate_reset) {
        ctx->gate->phase = 0.f;
        astrid_instrument_set_param_int32(instrument, PARAM_GATE_PHASE_RESET, 0);
    }

    for(i=0; i < blocksize; i++) {
        sample = 0.f;
        for(j=0; j < NUMOSCS; j++) {
            // TODO use this LFO for the freq drift -- use phase offsets to vary the drift amount
            //saturation = LPInterpolation.linear_pos(ctx->curves[j], ctx->curves[j]->phase) * 0.05f + 0.95f;
            ctx->oscs[j]->saturation = osc_saturation;
            ctx->oscs[j]->pulsewidth = osc_pw;
            // TODO apply the octave spread here, use the drift LFO to vary the spread
            ctx->oscs[j]->freq = freqs[j % num_freqs] * (osc_drift + 1.f);

            sample += LPPulsarOsc.process(ctx->oscs[j]) * LPInterpolation.linear_pos(ctx->env, ctx->env_phases[j]) * ((1.f/NUMOSCS)*3.f);

            ctx->env_phases[j] += ctx->env_phaseincs[j] * (1.f+osc_env_speed) * 3.f;

            if(ctx->env_phases[j] >= 1.f) {
                // something special on phase resets maybe?
            }

            while(ctx->env_phases[j] >= 1.f) ctx->env_phases[j] -= 1.f;

            ctx->curves[j]->phase += ctx->env_phaseincs[j];
            while(ctx->curves[j]->phase >= 1.f) ctx->curves[j]->phase -= 1.f;
        }

        sample *= amp;

        if(sample > 1.f || sample < -1.f) {
            sample = LPFX.fold(sample, &ctx->foldprev, instrument->samplerate);
        }

        sample = LPFX.limit(sample, &ctx->limitprev, 1.f, 0.2f, ctx->limitbuf);

        left = right = sample;

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
    ctx->foldprev = 0.f;
    ctx->limitprev = 1.f;
    ctx->limitbuf = LPBuffer.create(1024, 1, (lpfloat_t)SR);

    // setup oscs and curves
    for(int i=0; i < NUMOSCS; i++) {
        ctx->env_phases[i] = LPRand.rand(0.f, 1.f);
        ctx->env_phaseincs[i] = (1.f/SR) * LPRand.rand(0.005f, 0.03f) * 0.5f;
        ctx->curves[i] = LPWindow.create(WIN_RND, 4096);

        ctx->oscs[i] = LPPulsarOsc.create(4, 2,
            WT_SINE, WTSIZE, WT_TRI2, WTSIZE, 
            WT_TRI2, WTSIZE, WT_SINE, WTSIZE,   
            WIN_SINE, WTSIZE, WIN_HANN, WTSIZE 
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
    for(int i=0; i < MAXNUMFREQS; i++) {
        ctx->selected_freqs[i] = scale[LPRand.randint(0, MAXNUMFREQS*2) % MAXNUMFREQS] * 0.5f + LPRand.rand(0.f, 1.f);
    }
    astrid_instrument_set_param_float_list(instrument, PARAM_OSC_FREQS, ctx->selected_freqs, MAXNUMFREQS);

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
    LPBuffer.destroy(ctx->limitbuf);

    free(ctx);

    printf("Done!\n");
    return 0;
}
