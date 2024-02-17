#include "astrid.h"

#define SR 48000
#define CHANNELS 2

#define NUMOSCS (CHANNELS * 10)
#define NUMWINS 2
#define NUMWTS 2
#define WTSIZE 4096
#define NUMFREQS 12

lpinstrument_t instrument;
char name[] = "astrid-pulsar";
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
    lpbuffer_t * ringbuf;
    lpbuffer_t * env;

    lpbuffer_t * curves[NUMOSCS];
    lpfloat_t env_phases[NUMOSCS];
    lpfloat_t env_phaseincs[NUMOSCS];
} localctx_t;


void handle_shutdown(__attribute__((unused)) int sig) {
    instrument.is_running = 0;
}

lpbuffer_t * renderer_callback(lpinstrument_t * instrument) {
    lpbuffer_t * out;
    lpbuffer_t * snd;
    lpbuffer_t * speed;

    speed = LPWindow.create(WIN_RND, 4096);
    snd = LPSoundFile.read("chimeC.wav");
    LPBuffer.scale(speed, 0, 1, 0.9, 1.1);
    out = LPBuffer.varispeed(snd, speed);
    LPBuffer.multiply_scalar(out, LPRand.rand(0.1f, 0.5f));

    free(speed);
    free(snd);

    syslog(LOG_DEBUG, "done rendering %d frames and %d channels...\n", (int)out->length, out->channels);
    return out;
}

void audio_callback(int channels, size_t blocksize, float ** input, float ** output, void * arg) {
    size_t idx, i;
    int j, c;
    lpfloat_t freqs[NUMFREQS];
    lpfloat_t sample, amp, pw, saturation;
    localctx_t * ctx = (localctx_t *)arg;

    if(!instrument.is_running) return;

    for(i=0; i < blocksize; i++) {
        idx = (ctx->ringbuf->pos + i) % ctx->ringbuf->length;
        for(c=0; c < channels; c++) {
            ctx->ringbuf->data[idx * channels + c] = input[c][i];
        }
    }
    ctx->ringbuf->pos += blocksize;

    amp = astrid_instrument_get_param_float(&instrument, PARAM_AMP, 0.08f);
    pw = astrid_instrument_get_param_float(&instrument, PARAM_PW, 1.f);
    astrid_instrument_get_param_float_list(&instrument, PARAM_FREQS, NUMFREQS, freqs);

    for(i=0; i < blocksize; i++) {
        sample = 0.f;
        for(j=0; j < NUMOSCS; j++) {
            saturation = LPInterpolation.linear_pos(ctx->curves[j], ctx->curves[j]->phase);
            ctx->oscs[j]->saturation = saturation;
            ctx->oscs[j]->pulsewidth = pw;
            ctx->oscs[j]->freq = freqs[j % NUMFREQS] * 4.f;

            sample += LPPulsarOsc.process(ctx->oscs[j]) * amp * LPInterpolation.linear_pos(ctx->env, ctx->env_phases[j]) * 0.12f;

            ctx->env_phases[j] += ctx->env_phaseincs[j];

            if(ctx->env_phases[j] >= 1.f) {
                // env boundries
            }

            while(ctx->env_phases[j] >= 1.f) ctx->env_phases[j] -= 1.f;

            ctx->curves[j]->phase += ctx->env_phaseincs[j];
            while(ctx->curves[j]->phase >= 1.f) ctx->curves[j]->phase -= 1.f;
        }

        for(c=0; c < channels; c++) {
            //output[c][i] = (float)sample * input[c][i];
            output[c][i] = (float)sample * 0.5f;
        }
    }
}

int main() {
    lpfloat_t selected_freqs[NUMFREQS];
    const char * instrument_basename = "pulsar";
    //const char * python_script_path = "orc/pulsar.py";
    double processing_time_so_far, onset_delay_in_seconds;
    double now = 0;

    lpbuffer_t * buf;
    lpbuffer_t * wts;
    lpbuffer_t * win;
    size_t wt_onsets[NUMWTS];
    size_t wt_lengths[NUMWTS];
    size_t win_onsets[NUMWINS];
    size_t win_lengths[NUMWINS];

    // create local context struct
    localctx_t * ctx = (localctx_t *)calloc(1, sizeof(localctx_t));
    if(ctx == NULL) {
        printf("Could not alloc ctx: (%d) %s\n", errno, strerror(errno));
        exit(1);
    }

    // create env and ringbuf
    ctx->env = LPWindow.create(WIN_HANNOUT, 4096);
    ctx->ringbuf = LPBuffer.create(SR * 10, CHANNELS, SR);

    // create wt/win stacks
    wts = LPWavetable.create_stack(NUMWTS, 
            wt_onsets, wt_lengths,
            WT_SINE, WTSIZE,
            WT_TRI2, WTSIZE
    );
    win = LPWindow.create_stack(NUMWINS, 
            win_onsets, win_lengths,
            WIN_SINE, WTSIZE,
            WIN_HANN, WTSIZE
    );

    // setup oscs and curves
    for(int i=0; i < NUMOSCS; i++) {
        ctx->env_phases[i] = LPRand.rand(0.f, 1.f);
        ctx->env_phaseincs[i] = (1.f/SR) * LPRand.rand(0.001f, 0.1f);
        ctx->curves[i] = LPWindow.create(WIN_RND, 4096);

        ctx->oscs[i] = LPPulsarOsc.create(
            NUMWTS, wts, wt_onsets, wt_lengths, 
            NUMWINS, win, win_onsets, win_lengths
        );

        selected_freqs[i] = scale[LPRand.randint(0, NUMFREQS*2) % NUMFREQS] * 0.5f + LPRand.rand(0.f, 1.f);
        ctx->oscs[i]->samplerate = (lpfloat_t)SR;
        ctx->oscs[i]->wavetable_morph_freq = LPRand.rand(0.001f, 0.15f);
        ctx->oscs[i]->phase = 0.f;
    }

    instrument.callback = audio_callback;
    instrument.context = (void*)ctx;
    instrument.shutdown = handle_shutdown;
    instrument.channels = CHANNELS; // FIXME merge all this together whydoncha

    if(setup_async_mixer(&instrument) < 0) {
        printf("Failed to set up async mixer: (%d) %s\n", errno, strerror(errno));
        exit(1);
    }

    if(astrid_instrument_audio_start(instrument_basename, CHANNELS, &instrument) < 0) {
        printf("Could not start instrument: (%d) %s\n", errno, strerror(errno));
        exit(1);
    }

    // set initial freqs
    astrid_instrument_set_param_float_list(&instrument, PARAM_FREQS, selected_freqs, NUMFREQS);

    while(instrument.is_running) {
        printf("Waiting for messages...\n");

        astrid_instrument_msg_read_next(&instrument);

        switch(instrument.msg.type) {
            case LPMSG_PLAY:
                syslog(LOG_DEBUG, "MSG: play\n");
                break;

            case LPMSG_UPDATE:
                for(int i=0; i < NUMFREQS; i++) {
                    selected_freqs[i] = scale[LPRand.randint(0, NUMFREQS*2) % NUMFREQS] * 0.5f + LPRand.rand(0.f, 1.f);
                }
                astrid_instrument_set_param_float_list(&instrument, PARAM_FREQS, selected_freqs, NUMFREQS);
                astrid_instrument_set_param_float(&instrument, PARAM_AMP, LPRand.rand(0.5f, 1.f));
                astrid_instrument_set_param_float(&instrument, PARAM_PW, LPRand.rand(0.05f, 1.f));

                syslog(LOG_DEBUG, "MSG: update | %s\n", instrument.msg.msg);
                break;

            case LPMSG_LOAD:
                syslog(LOG_DEBUG, "MSG: load\n");
                break;

            case LPMSG_TRIGGER:
                syslog(LOG_DEBUG, "MSG: trigger\n");

                /* Do the render */
                buf = renderer_callback(&instrument);

                if(buf == NULL) {
                    syslog(LOG_ERR, "null buffer\n");
                    continue;
                }

                syslog(LOG_DEBUG, "rendered buffer is %d frames and %d channels...\n", (int)buf->length, buf->channels);

                /* Get now */
                if(lpscheduler_get_now_seconds(&now) < 0) {
                    syslog(LOG_ERR, "Could not get now seconds for loop retriggering\n");
                    now = 0;
                }

                processing_time_so_far = now - instrument.msg.initiated;
                onset_delay_in_seconds = instrument.msg.scheduled - processing_time_so_far;
                if(onset_delay_in_seconds < 0) onset_delay_in_seconds = 0.f;

                instrument.msg.onset_delay = (size_t)(onset_delay_in_seconds * ASTRID_SAMPLERATE);

                syslog(LOG_INFO, "msg.onset_delay %ld\n", instrument.msg.onset_delay);

                /* Schedule the buffer for playback */
                scheduler_schedule_event(instrument.async_mixer, buf, 0);
                continue;

            case LPMSG_SHUTDOWN:
                syslog(LOG_DEBUG, "MSG: shutdown\n");
                instrument.is_running = 0;
                break;

            default:
                continue;
        }
    }

    printf("Cleaning up...\n");

    astrid_instrument_audio_stop(&instrument);
    if(instrument.async_mixer != NULL) scheduler_destroy(instrument.async_mixer);

    for(int o=0; o < NUMOSCS; o++) {
        LPPulsarOsc.destroy(ctx->oscs[o]);
        LPBuffer.destroy(ctx->curves[o]);
    }

    LPBuffer.destroy(ctx->ringbuf);
    LPBuffer.destroy(ctx->env);
    LPBuffer.destroy(win);
    LPBuffer.destroy(wts);

    free(ctx);

    printf("Done!\n");
    return 0;
}
