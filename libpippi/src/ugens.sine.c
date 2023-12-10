#include "pippi.h"
#include "ugens.sine.h"
#include "oscs.sine.h"


void process_sine_ugen(ugen_t * u) {
    lpugensine_t * params;
    params = (lpugensine_t *)u->params;
    params->outputs[USINEOUT_MAIN] = LPSineOsc.process(params->osc);
    params->outputs[USINEOUT_FREQ] = params->osc->freq;
    params->outputs[USINEOUT_PHASE] = params->osc->phase;
}

void destroy_sine_ugen(ugen_t * u) {
    lpugensine_t * params;
    params = (lpugensine_t *)u->params;
    free(params->osc);
    free(params);
    free(u);
}

void set_sine_ugen_param(ugen_t * u, int index, lpfloat_t value) {
    lpugensine_t * params;
    params = (lpugensine_t *)u->params;
    if(index == USINEIN_FREQ) params->osc->freq = value;
    if(index == USINEIN_PHASE) params->osc->phase = value;
}

lpfloat_t get_sine_ugen_output(ugen_t * u, int index) {
    lpugensine_t * params;
    params = (lpugensine_t *)u->params;
    return params->outputs[index];
}

ugen_t * create_sine_ugen(void) {
    ugen_t * u;
    lpugensine_t * params;

    params = (lpugensine_t *)calloc(sizeof(lpugensine_t), 1);
    params->osc = LPSineOsc.create();
    u = (ugen_t *)calloc(sizeof(ugen_t), 1);

    u->params = (void *)params;
    u->process = process_sine_ugen;
    u->destroy = destroy_sine_ugen;
    u->get_output = get_sine_ugen_output;
    u->set_param = set_sine_ugen_param;

    return u;
}

