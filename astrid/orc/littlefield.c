#include "astrid.h"

#define NAME "littlefield"

#define SR 48000
#define CHANNELS 2
#define ADC_LENGTH 30
#define RESAMPLER_LENGTH 30
#define MIC_ATTENUATION 0.2f

#define NUMOSCS 10
#define WTSIZE 4096
#define MAXNUMFREQS NUMOSCS

int num_freqs = MAXNUMFREQS;

lpfloat_t terry[] = {
    1.f,         // P1  C
    16.f/15.f,   // m2  Db
    10.f/9.f,    // M2  D
    6.f/5.f,     // m3  Eb
    5.f/4.f,     // M3  E
    4.f/3.f,     // P4  F
    64.f/45.f,   // TT  Gb
    3.f/2.f,     // P5  G
    8.f/5.f,     // m6  Ab
    27.f/16.f,   // M6  A 
    16.f/9.f,    // m7  Bb
    15.f/8.f,    // M7  B
};

// this is just used as the default freq list on startup
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

enum DistortionTypes {
    DISTORTION_FOLDBACK,
    DISTORTION_BITCRUSH,
    NUM_DISTORTIONS,
};

enum InstrumentParams {
    PARAM_NOOP,
    PARAM_OSC_AMP,
    PARAM_OSC_PULSEWIDTH,
    PARAM_OSC_SATURATION,
    PARAM_OSC_ENVELOPE_SPEED,
    PARAM_OSC_DRIFT_DEPTH,
    PARAM_OSC_DRIFT_SPEED,
    PARAM_OSC_OCTAVE_SPREAD,
    PARAM_OSC_OCTAVE_OFFSET,
    PARAM_OSC_FREQ_UPDATE_ON_ENV_RESET,
    PARAM_OSC_FREQS,

    PARAM_GATE1_AMP,
    PARAM_GATE2_AMP,
    PARAM_GATE1_SPEED,
    PARAM_GATE2_SPEED,
    PARAM_GATE_SPEED_MULTIPLIER,
    PARAM_GATE_PULSEWIDTH,
    PARAM_GATE_SATURATION,
    PARAM_GATE_SHAPE,
    PARAM_GATE_DRIFT_DEPTH,
    PARAM_GATE_DRIFT_SPEED,
    PARAM_GATE_DRIFT_STABILITY,
    PARAM_GATE_DRIFT_DENSITY,
    PARAM_GATE_DRIFT_PERIODICITY,
    PARAM_GATE_PATTERN,
    PARAM_GATE_PHASE_RESET,
    PARAM_GATE_REPEAT,

    PARAM_IN1_TO_OUT1,
    PARAM_IN1_TO_OUT2,
    PARAM_IN2_TO_OUT1,
    PARAM_IN2_TO_OUT2,

    PARAM_OSC_TO_OUT1,
    PARAM_OSC_TO_OUT2,
    PARAM_GATE_TO_OUT1,
    PARAM_GATE_TO_OUT2,

    PARAM_DISTORTION_TYPE,
    PARAM_DISTORTION_AMOUNT,
    PARAM_DISTORTION_MIX,

    PARAM_MIC_PITCH_TRACKING_THRESHOLD,
    PARAM_MIC_PITCH_TRACKING_ENABLED,
    NUMPARAMS
};

typedef struct localctx_t {
    lppulsarosc_t * oscs[NUMOSCS];
    lpyin_t * yin;
    lpfloat_t freqsmooth;
    lpenvelopefollower_t * envf;

    lpfloat_t ampmodsmooth;
    lpfloat_t oscampsmooth;
    lpfloat_t gateampsmooth;

    lpfloat_t fold1prev;
    lpfloat_t fold2prev;

    lpfloat_t limit1prev;
    lpfloat_t limit2prev;
    lpbuffer_t * limit1buf;
    lpbuffer_t * limit2buf;

    lppulsarosc_t * gate1;
    lppulsarosc_t * gate2;
    lpshapeosc_t * gate_drifter;
    lpbuffer_t * gate_drift_win;

    lpbuffer_t * env;
    lpbuffer_t * drifters[NUMOSCS];
    lpfloat_t drift_phaseincs[NUMOSCS];

    lpfloat_t env_phases[NUMOSCS];
    lpfloat_t env_phaseincs[NUMOSCS];
    lpfloat_t octave_offsets[NUMOSCS];
    lpfloat_t octave_spreads[NUMOSCS];
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
    lpfloat_t val_floatlist[MAXNUMFREQS] = {220.};
    float val_f = 0;
    int32_t val_i32 = 0;
    lppatternbuf_t val_pattern = {1,{1}};

    syslog(LOG_DEBUG, "Got param %s=%s\n", keystr, valstr);

    if(strcmp(keystr, "oamp") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_AMP, val_f);
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
    } else if(strcmp(keystr, "odspeed") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_DRIFT_SPEED, val_f);
    } else if(strcmp(keystr, "octspread") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        astrid_instrument_set_param_int32(instrument, PARAM_OSC_OCTAVE_SPREAD, val_i32);
    } else if(strcmp(keystr, "octoffset") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        astrid_instrument_set_param_int32(instrument, PARAM_OSC_OCTAVE_OFFSET, val_i32);
    } else if(strcmp(keystr, "freqsnap") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        astrid_instrument_set_param_int32(instrument, PARAM_OSC_FREQ_UPDATE_ON_ENV_RESET, val_i32);
    } else if(strcmp(keystr, "freqs") == 0) {
        num_freqs = extract_floatlist_from_token(valstr, val_floatlist, MAXNUMFREQS);
        astrid_instrument_set_param_float_list(instrument, PARAM_OSC_FREQS, val_floatlist, num_freqs);
    } else if(strcmp(keystr, "oo1") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_TO_OUT1, val_f);
    } else if(strcmp(keystr, "oo2") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_OSC_TO_OUT2, val_f);

    } else if(strcmp(keystr, "g1amp") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE1_AMP, val_f);
    } else if(strcmp(keystr, "g1speed") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE2_SPEED, val_f);
    } else if(strcmp(keystr, "g2amp") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE2_AMP, val_f);
    } else if(strcmp(keystr, "g2speed") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE2_SPEED, val_f);

    } else if(strcmp(keystr, "gmult") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_SPEED_MULTIPLIER, val_f);
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
   } else if(strcmp(keystr, "gdspeed") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_DRIFT_SPEED, val_f);
   } else if(strcmp(keystr, "gdstab") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_DRIFT_STABILITY, val_f);
   } else if(strcmp(keystr, "gdden") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_DRIFT_DENSITY, val_f);
   } else if(strcmp(keystr, "gdper") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_DRIFT_PERIODICITY, val_f);

    } else if(strcmp(keystr, "go1") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_TO_OUT1, val_f);
    } else if(strcmp(keystr, "go2") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_GATE_TO_OUT2, val_f);

    } else if(strcmp(keystr, "pat") == 0) {
        extract_patternbuf_from_token(valstr, val_pattern.pattern, &val_pattern.length);
        astrid_instrument_set_param_patternbuf(instrument, PARAM_GATE_PATTERN, &val_pattern);
    } else if(strcmp(keystr, "greset") == 0) {
        astrid_instrument_set_param_int32(instrument, PARAM_GATE_PHASE_RESET, 1);
    } else if(strcmp(keystr, "grep") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        astrid_instrument_set_param_int32(instrument, PARAM_GATE_REPEAT, val_i32);

    } else if(strcmp(keystr, "i1o1") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_IN1_TO_OUT1, val_f);
    } else if(strcmp(keystr, "i1o2") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_IN1_TO_OUT2, val_f);
    } else if(strcmp(keystr, "i2o1") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_IN2_TO_OUT1, val_f);
    } else if(strcmp(keystr, "i2o2") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_IN2_TO_OUT2, val_f);

    } else if(strcmp(keystr, "disttype") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        if(val_i32 < 0 || val_i32 >= NUM_DISTORTIONS) val_i32 = DISTORTION_FOLDBACK;
        astrid_instrument_set_param_int32(instrument, PARAM_DISTORTION_TYPE, val_i32);
    } else if(strcmp(keystr, "distmix") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_DISTORTION_MIX, val_f);
    } else if(strcmp(keystr, "dist") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_DISTORTION_AMOUNT, val_f);

    } else if(strcmp(keystr, "mtrak") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        astrid_instrument_set_param_int32(instrument, PARAM_MIC_PITCH_TRACKING_ENABLED, val_i32);
    } else if(strcmp(keystr, "pthresh") == 0) {
        extract_float_from_token(valstr, &val_f);
        astrid_instrument_set_param_float(instrument, PARAM_MIC_PITCH_TRACKING_THRESHOLD, val_f);


    } else if(strcmp(keystr, "save") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        astrid_instrument_save_param_session_snapshot(instrument, NUMPARAMS, val_i32);
    } else if(strcmp(keystr, "restore") == 0) {
        extract_int32_from_token(valstr, &val_i32);
        astrid_instrument_restore_param_session_snapshot(instrument, val_i32);
    }    

    return 0;
}

// FIXME is this actually useful, or is it better to just do all the async stuff in python?
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
    lpfloat_t osc_sample, osc_sample_group1, osc_sample_group2, 
              osc_amp, env1, env2, in1, in2, d1, d2, p, last_p=-1,
              distortion_amount, distortion_mix,
              osc_pw, osc_saturation, osc_env_speed, 
              osc_drift_amount, drift, osc_drift_speed,
              osc_out1_mix, osc_out2_mix, gate_out1_mix, gate_out2_mix,
              gate1_sample, gate2_sample, gate1_amp, gate2_amp, amp_mod,
              in1_out1_mix, in1_out2_mix, in2_out1_mix, in2_out2_mix,
              gate_shape, gate1_speed, gate2_speed, gate_speed_multiplier, 
              gate_pw, gate_drift_amount, gate_drift_speed,
              gate_drift_stability, gate_drift_density, gate_drift_periodicity,
              gate_drift, gate_saturation, tracked_freq=220.f;
    int32_t octave_spread, octave_offset, gate_reset, gate_repeat, 
            freq_snap, pitch_tracking_is_enabled, distortion_type;
    lppatternbuf_t gate_pattern;
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    localctx_t * ctx = (localctx_t *)instrument->context;

    if(!instrument->is_running) return 1;

    distortion_amount = astrid_instrument_get_param_float(instrument, PARAM_DISTORTION_AMOUNT, 0.f);
    distortion_mix = astrid_instrument_get_param_float(instrument, PARAM_DISTORTION_MIX, 0.f);
    distortion_type = astrid_instrument_get_param_int32(instrument, PARAM_DISTORTION_TYPE, 0);

    osc_amp = astrid_instrument_get_param_float(instrument, PARAM_OSC_AMP, 1.f);
    osc_amp = LPFX.lpf1(osc_amp, &ctx->oscampsmooth, 1000.f, SR);
    osc_out1_mix = astrid_instrument_get_param_float(instrument, PARAM_OSC_TO_OUT1, 0.5f);
    osc_out2_mix = astrid_instrument_get_param_float(instrument, PARAM_OSC_TO_OUT2, 0.5f);
    osc_pw = astrid_instrument_get_param_float(instrument, PARAM_OSC_PULSEWIDTH, 1.f);
    osc_saturation = astrid_instrument_get_param_float(instrument, PARAM_OSC_SATURATION, 1.f);
    osc_env_speed = astrid_instrument_get_param_float(instrument, PARAM_OSC_ENVELOPE_SPEED, 0.001f) * 100.f + 1.f;
    osc_drift_amount = astrid_instrument_get_param_float(instrument, PARAM_OSC_DRIFT_DEPTH, 0.f);
    osc_drift_speed = astrid_instrument_get_param_float(instrument, PARAM_OSC_DRIFT_SPEED, 0.5f);
    octave_spread = astrid_instrument_get_param_int32(instrument, PARAM_OSC_OCTAVE_SPREAD, 6);
    octave_offset = astrid_instrument_get_param_int32(instrument, PARAM_OSC_OCTAVE_OFFSET, -2);
    freq_snap = astrid_instrument_get_param_int32(instrument, PARAM_OSC_FREQ_UPDATE_ON_ENV_RESET, 0);
    astrid_instrument_get_param_float_list(instrument, PARAM_OSC_FREQS, num_freqs, freqs);

    gate1_amp = astrid_instrument_get_param_float(instrument, PARAM_GATE1_AMP, 0.f);
    gate1_amp = LPFX.lpf1(gate1_amp, &ctx->gateampsmooth, 1000.f, SR);

    gate2_amp = astrid_instrument_get_param_float(instrument, PARAM_GATE2_AMP, 0.f);
    gate2_amp = LPFX.lpf1(gate2_amp, &ctx->gateampsmooth, 1000.f, SR);

    gate_out1_mix = astrid_instrument_get_param_float(instrument, PARAM_GATE_TO_OUT1, 0.5f);
    gate_out2_mix = astrid_instrument_get_param_float(instrument, PARAM_GATE_TO_OUT2, 0.5f);
    gate_reset = astrid_instrument_get_param_int32(instrument, PARAM_GATE_PHASE_RESET, 0);
    gate_repeat = astrid_instrument_get_param_int32(instrument, PARAM_GATE_REPEAT, 1);
    gate_shape = astrid_instrument_get_param_float(instrument, PARAM_GATE_SHAPE, 0.f);
    gate1_speed = astrid_instrument_get_param_float(instrument, PARAM_GATE1_SPEED, 2.f);
    gate2_speed = astrid_instrument_get_param_float(instrument, PARAM_GATE2_SPEED, 2.f);
    gate_speed_multiplier = astrid_instrument_get_param_float(instrument, PARAM_GATE_SPEED_MULTIPLIER, 1.f);
    gate_drift_amount = astrid_instrument_get_param_float(instrument, PARAM_GATE_DRIFT_DEPTH, 0.f);
    gate_drift_speed = astrid_instrument_get_param_float(instrument, PARAM_GATE_DRIFT_SPEED, 1.f) * 100.f;
    gate_drift_stability = astrid_instrument_get_param_float(instrument, PARAM_GATE_DRIFT_STABILITY, 0.1f) * 0.5f;
    gate_drift_density = astrid_instrument_get_param_float(instrument, PARAM_GATE_DRIFT_DENSITY, 1.f);
    gate_drift_periodicity = astrid_instrument_get_param_float(instrument, PARAM_GATE_DRIFT_PERIODICITY, 0.5f);
    gate_saturation = astrid_instrument_get_param_float(instrument, PARAM_GATE_SATURATION, 1.f);
    gate_pw = astrid_instrument_get_param_float(instrument, PARAM_GATE_PULSEWIDTH, 1.f);
    gate_pattern = astrid_instrument_get_param_patternbuf(instrument, PARAM_GATE_PATTERN);

    in1_out1_mix = astrid_instrument_get_param_float(instrument, PARAM_IN1_TO_OUT1, 0.f);
    in1_out2_mix = astrid_instrument_get_param_float(instrument, PARAM_IN1_TO_OUT2, 0.f);
    in2_out1_mix = astrid_instrument_get_param_float(instrument, PARAM_IN2_TO_OUT1, 0.f);
    in2_out2_mix = astrid_instrument_get_param_float(instrument, PARAM_IN2_TO_OUT2, 0.f);

    pitch_tracking_is_enabled = astrid_instrument_get_param_int32(instrument, PARAM_MIC_PITCH_TRACKING_ENABLED, 0);
    //ctx->yin->threshold = astrid_instrument_get_param_int32(instrument, PARAM_MIC_PITCH_TRACKING_THRESHOLD, 0.8f);

    // set gate osc params
    ctx->gate_drifter->freq = gate_drift_speed;
    ctx->gate_drifter->minfreq = gate_drift_speed * 0.1f;
    ctx->gate_drifter->maxfreq = gate_drift_speed * 10.f;
    ctx->gate_drifter->stability = gate_drift_stability;
    ctx->gate_drifter->density = gate_drift_density;
    ctx->gate_drifter->periodicity = gate_drift_periodicity;
    gate_drift = LPShapeOsc.process(ctx->gate_drifter) * gate_drift_amount;

    ctx->gate1->window_morph = fmin(gate_shape, 0.999f);
    ctx->gate1->freq = (gate1_speed * gate_speed_multiplier) + gate_drift;
    ctx->gate1->pulsewidth = gate_pw;
    ctx->gate1->saturation = gate_saturation;
    LPPulsarOsc.burst_bytes(ctx->gate1, gate_pattern.pattern, gate_pattern.length);
    ctx->gate1->once = !gate_repeat;

    ctx->gate2->window_morph = fmin(gate_shape, 0.999f);
    ctx->gate2->freq = (gate2_speed * gate_speed_multiplier) + gate_drift;
    ctx->gate2->pulsewidth = gate_pw;
    ctx->gate2->saturation = gate_saturation;
    LPPulsarOsc.burst_bytes(ctx->gate2, gate_pattern.pattern, gate_pattern.length);
    ctx->gate2->once = !gate_repeat;

    if(gate_reset) {
        ctx->gate1->phase = 0.f;
        ctx->gate2->phase = 0.f;
        astrid_instrument_set_param_int32(instrument, PARAM_GATE_PHASE_RESET, 0);
    }

    for(i=0; i < blocksize; i++) {
        osc_sample_group1 = osc_sample_group2 = d1 = d2 = 0.f;

        amp_mod = LPEnvelopeFollower.process(ctx->envf, input[0][i]);
        amp_mod = LPFX.lpf1(amp_mod, &ctx->ampmodsmooth, 100.f, SR);

        // track the pitch if it's enabled
        if(pitch_tracking_is_enabled) {
            p = LPPitchTracker.yin_process(ctx->yin, input[0][i]);
            if(p > 0 && p != last_p) {
                tracked_freq = p;
                last_p = p;
            }

            tracked_freq = LPFX.lpf1(p, &ctx->freqsmooth, 500.f, SR);

            // track in a ~melodic range
            //while(tracked_freq >= 800) tracked_freq *= 0.5f;
            //while(tracked_freq <= 80) tracked_freq *= 2.f;
        }

        for(j=0; j < NUMOSCS; j++) {
            ctx->oscs[j]->saturation = osc_saturation;
            ctx->oscs[j]->pulsewidth = osc_pw;

            drift = LPInterpolation.linear_pos(ctx->drifters[j], ctx->drifters[j]->phase) * osc_drift_amount;

            // set the freq to the freqs table pitch or tracked pitch if tracking is enabled
            if(!freq_snap) {
                ctx->octave_offsets[j] = pow(2, octave_offset);
            }

            ctx->oscs[j]->freq = freqs[j % num_freqs];
            if(pitch_tracking_is_enabled) ctx->oscs[j]->freq = tracked_freq;
            ctx->oscs[j]->freq *= (ctx->octave_offsets[j] * ctx->octave_spreads[j]) + drift;

            // mix in the enveloped osc voice
            if(j % 2 == 0) {
                osc_sample_group1 += LPPulsarOsc.process(ctx->oscs[j]) * LPInterpolation.linear_pos(ctx->env, ctx->env_phases[j]) * ((1.f/NUMOSCS)*3.f);
            } else {
                osc_sample_group2 += LPPulsarOsc.process(ctx->oscs[j]) * LPInterpolation.linear_pos(ctx->env, ctx->env_phases[j]) * ((1.f/NUMOSCS)*3.f);
            }

            // advance the voice envelope phase
            ctx->env_phases[j] += ctx->env_phaseincs[j] * (1.f+osc_env_speed) * 3.f;

            // on voice envelope phase resets, recalculate the octave multiplier
            if(ctx->env_phases[j] >= 1.f) {
                octave_spread = LPRand.randint(-octave_spread, octave_spread);
                ctx->octave_spreads[j] = pow(2, octave_spread);
                ctx->octave_offsets[j] = pow(2, octave_offset);

                /*
                syslog(LOG_ERR, "GDRIFT=%f GDRIFTSPEED=%f GDRIFTAMOUNT=%f\n",
                        gate_drift, gate_drift_speed, gate_drift_amount);
                syslog(LOG_ERR, "VALUE %f\n", ctx->envf->value);
                syslog(LOG_ERR, "AMPMOD %f\n", amp_mod);
                */
            }
            while(ctx->env_phases[j] >= 1.f) ctx->env_phases[j] -= 1.f;

            // advance the osc freq drift LFO phases
            ctx->drifters[j]->phase += ctx->drift_phaseincs[j] * osc_drift_speed;
            while(ctx->drifters[j]->phase >= 1.f) ctx->drifters[j]->phase -= 1.f;
        }

        // gated output
        env1 = LPPulsarOsc.process(ctx->gate1);
        gate1_sample = osc_sample_group1 * env1 * gate1_amp;

        env2 = LPPulsarOsc.process(ctx->gate2);
        gate2_sample = osc_sample_group2 * env2 * gate2_amp;

        // osc output
        osc_sample = osc_sample_group1 + osc_sample_group2;
        osc_sample *= osc_amp * amp_mod;

        // mic feedback
        in1 = input[0][i] * MIC_ATTENUATION;
        in2 = input[1][i] * MIC_ATTENUATION;

        // mix everything
        output[0][i] += (osc_sample * osc_out1_mix) + ((gate1_sample + gate2_sample) * gate_out1_mix) + (in1 * in1_out1_mix) + (in2 * in2_out1_mix);
        output[1][i] += (osc_sample * osc_out2_mix) + ((gate1_sample + gate2_sample) * gate_out2_mix) + (in2 * in1_out2_mix) + (in2 * in2_out2_mix);

        // apply distortion
        if(distortion_amount > 0) {
            switch(distortion_type) {
                case DISTORTION_FOLDBACK:
                    d1 = LPFX.fold(output[0][i] * (distortion_amount+1), &ctx->fold1prev, instrument->samplerate);
                    d2 = LPFX.fold(output[1][i] * (distortion_amount+1), &ctx->fold2prev, instrument->samplerate);
                    break;

                case DISTORTION_BITCRUSH:
                    d1 = LPFX.crush(output[0][i], (1.f-distortion_amount) * 15 + 1);
                    d2 = LPFX.crush(output[1][i], (1.f-distortion_amount) * 15 + 1);
                    break;

                default:
                    break;
            }

            output[0][i] += d1 * distortion_mix;
            output[1][i] += d2 * distortion_mix;
        }

        // limit
        output[0][i] = LPFX.limit(output[0][i], &ctx->limit1prev, 1.f, 0.2f, ctx->limit1buf);
        output[1][i] = LPFX.limit(output[1][i], &ctx->limit2prev, 1.f, 0.2f, ctx->limit2buf);
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
    ctx->fold1prev = 0.f;
    ctx->fold2prev = 0.f;
    ctx->limit1prev = 1.f;
    ctx->limit2prev = 1.f;
    ctx->limit1buf = LPBuffer.create(1024, 1, (lpfloat_t)SR);
    ctx->limit2buf = LPBuffer.create(1024, 1, (lpfloat_t)SR);

    // setup oscs
    for(int i=0; i < NUMOSCS; i++) {
        ctx->env_phases[i] = LPRand.rand(0.f, 1.f);
        ctx->env_phaseincs[i] = (1.f/SR) * LPRand.rand(0.005f, 0.03f) * 0.5f;
        ctx->drifters[i] = LPWavetable.create(WT_RND, 4096);
        ctx->drift_phaseincs[i] = (1.f/SR) * LPRand.rand(0.005f, 0.03f);
        ctx->octave_offsets[i] = 1.f;
        ctx->octave_spreads[i] = 1.f;

        ctx->oscs[i] = LPPulsarOsc.create(4, 2,
            WT_SINE, WTSIZE, WT_TRI2, WTSIZE, 
            WT_TRI2, WTSIZE, WT_SINE, WTSIZE,   
            WIN_SINE, WTSIZE, WIN_HANN, WTSIZE 
        );

        ctx->oscs[i]->samplerate = (lpfloat_t)SR;
        ctx->oscs[i]->wavetable_morph_freq = LPRand.rand(0.001f, 0.15f);
        ctx->oscs[i]->phase = 0.f;
    }

    ctx->gate1 = LPPulsarOsc.create(0, 4,
        WIN_SINEOUT, WTSIZE, 
        WIN_SINE, WTSIZE, 
        WIN_PHASOR, WTSIZE, 
        WIN_SINEOUT, WTSIZE
    );

    ctx->gate2 = LPPulsarOsc.create(0, 4,
        WIN_SINEOUT, WTSIZE, 
        WIN_SINE, WTSIZE, 
        WIN_PHASOR, WTSIZE, 
        WIN_SINEOUT, WTSIZE
    );

    ctx->yin = LPPitchTracker.yin_create(4096, SR);
    ctx->yin->fallback = 220.f;

    ctx->envf = LPEnvelopeFollower.create(12.f, SR);

    ctx->gate1->samplerate = (lpfloat_t)SR;
    ctx->gate1->window_morph_freq = 0;
    ctx->gate1->freq = 30.f;
    ctx->gate1->phase = 0.f;

    ctx->gate2->samplerate = (lpfloat_t)SR;
    ctx->gate2->window_morph_freq = 0;
    ctx->gate2->freq = 30.f;
    ctx->gate2->phase = 0.f;

    ctx->gate_drift_win = LPWavetable.create(WT_SINE, 4096);
    ctx->gate_drifter = LPShapeOsc.create(ctx->gate_drift_win);

    // Set the callbacks for streaming, async renders and param updates
    if((instrument = astrid_instrument_start(NAME, CHANNELS, 0, ADC_LENGTH, RESAMPLER_LENGTH, (void*)ctx, 
                    NULL, NULL, audio_callback, renderer_callback, param_map_callback, NULL)) == NULL) {
        fprintf(stderr, "Could not start instrument: (%d) %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* now that LMDB is running, populate the initial freqs */
    astrid_instrument_set_param_float_list(instrument, PARAM_OSC_FREQS, scale, 12);

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
        LPBuffer.destroy(ctx->drifters[o]);
    }

    LPPulsarOsc.destroy(ctx->gate1);
    LPPulsarOsc.destroy(ctx->gate2);
    LPShapeOsc.destroy(ctx->gate_drifter);
    LPBuffer.destroy(ctx->gate_drift_win);
    LPBuffer.destroy(ctx->env);
    LPBuffer.destroy(ctx->limit1buf);
    LPBuffer.destroy(ctx->limit2buf);

    free(ctx);

    printf("Done!\n");
    return 0;
}
