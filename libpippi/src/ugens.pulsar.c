#include "pippi.h"
#include "ugens.pulsar.h"
#include "oscs.pulsar.h"


void process_pulsar_ugen(ugen_t * u) {
    lpugenpulsar_t * params;
    lpfloat_t sample;
    params = (lpugenpulsar_t *)u->params;
    sample = LPPulsarOsc.process(params->osc);

    params->outputs[UPULSAROUT_MAIN] = sample;
    params->outputs[UPULSAROUT_FREQ] = params->osc->freq;
    params->outputs[UPULSAROUT_PHASE] = params->osc->phase;
}

void destroy_pulsar_ugen(ugen_t * u) {
    lpugenpulsar_t * params;
    params = (lpugenpulsar_t *)u->params;
    free(params->osc);
    free(params);
    free(u);
}

void set_pulsar_ugen_param(ugen_t * u, int index, void * value) {
    int * i;
    size_t * s;
    size_t ** sp;
    lpfloat_t * v;
    lpfloat_t ** vp;
    lpugenpulsar_t * params;

    params = (lpugenpulsar_t *)u->params;
    switch(index) {
        case UPULSARIN_FREQ:
            v = (lpfloat_t *)value;
            params->osc->freq = *v;
            break;

        case UPULSARIN_PHASE:
            v = (lpfloat_t *)value;
            params->osc->phase = *v;
            break;

        case UPULSARIN_PULSEWIDTH:
            v = (lpfloat_t *)value;
            params->osc->pulsewidth = *v;
            break;

        case UPULSARIN_SATURATION:
            v = (lpfloat_t *)value;
            params->osc->saturation = *v;
            break;

        case UPULSARIN_WTTABLE:
            vp = (lpfloat_t **)value;
            params->osc->wavetables = *vp;
            break;

        case UPULSARIN_WTTABLELENGTH:
            s = (size_t *)value;
            params->osc->wavetable_length = *s;
            break;

        case UPULSARIN_NUMWTS:
            i = (int *)value;
            params->osc->num_wavetables = *i;
            break;

        case UPULSARIN_WTOFFSETS:
            sp = (size_t **)value;
            params->osc->wavetable_onsets = *sp;
            break;

        case UPULSARIN_WTLENGTHS:
            sp = (size_t **)value;
            params->osc->wavetable_lengths = *sp;
            break;

        case UPULSARIN_WTMORPH:
            v = (lpfloat_t *)value;
            params->osc->wavetable_morph = *v;
            break;

        case UPULSARIN_WTMORPHFREQ:
            v = (lpfloat_t *)value;
            params->osc->wavetable_morph_freq = *v;
            break;
            
        case UPULSARIN_WINTABLE:
            vp = (lpfloat_t **)value;
            params->osc->windows = *vp;
            break;

        case UPULSARIN_WINTABLELENGTH:
            s = (size_t *)value;
            params->osc->window_length = *s;
            break;

        case UPULSARIN_NUMWINS:
            i = (int *)value;
            params->osc->num_windows = *i;
            break;

        case UPULSARIN_WINOFFSETS:
            sp = (size_t **)value;
            params->osc->window_onsets = *sp;
            break;

        case UPULSARIN_WINLENGTHS:
            sp = (size_t **)value;
            params->osc->window_lengths = *sp;
            break;

        case UPULSARIN_WINMORPH:
            v = (lpfloat_t *)value;
            params->osc->window_morph = *v;
            break;

        case UPULSARIN_WINMORPHFREQ:
            v = (lpfloat_t *)value;
            params->osc->window_morph_freq = *v;
            break;

        default:
            break;
    }
}

lpfloat_t get_pulsar_ugen_output(ugen_t * u, int index) {
    lpugenpulsar_t * params;
    params = (lpugenpulsar_t *)u->params;
    return params->outputs[index];
}

ugen_t * create_pulsar_ugen(void) {
    ugen_t * u;
    lpugenpulsar_t * params;

    params = (lpugenpulsar_t *)LPMemoryPool.alloc(sizeof(lpugenpulsar_t), 1);
    params->osc = LPPulsarOsc.create(
        0, NULL, 0, NULL, NULL, 
        0, NULL, 0, NULL, NULL
    );

    u = (ugen_t *)LPMemoryPool.alloc(sizeof(ugen_t), 1);

    u->params = (void *)params;
    u->process = process_pulsar_ugen;
    u->destroy = destroy_pulsar_ugen;
    u->get_output = get_pulsar_ugen_output;
    u->set_param = set_pulsar_ugen_param;

    return u;
}

