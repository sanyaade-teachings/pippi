#include "pippi.h"
#include "ugens.utils.h"

void process_mult_ugen(ugen_t * u) {
    lpugenmult_t * params;
    params = (lpugenmult_t *)u->params;

    params->outputs[UMULTOUT_MAIN] = params->a * params->b;
    params->outputs[UMULTOUT_A] = params->a;
    params->outputs[UMULTOUT_B] = params->b;
}

void destroy_mult_ugen(ugen_t * u) {
    lpugenmult_t * params;
    params = (lpugenmult_t *)u->params;
    free(params);
    free(u);
}

void set_mult_ugen_param(ugen_t * u, int index, void * value) {
    lpugenmult_t * params;
    lpfloat_t * v;
    v = (lpfloat_t *)value;
    params = (lpugenmult_t *)u->params;
    if(index == UMULTIN_A) params->a = *v;
    if(index == UMULTIN_B) params->b = *v;
}

lpfloat_t get_mult_ugen_output(ugen_t * u, int index) {
    lpugenmult_t * params;
    params = (lpugenmult_t *)u->params;
    return params->outputs[index];
}

ugen_t * create_mult_ugen(void) {
    ugen_t * u;
    lpugenmult_t * params;

    u = (ugen_t *)calloc(sizeof(ugen_t), 1);
    params = (lpugenmult_t *)calloc(sizeof(lpugenmult_t), 1);
    params->a = 1;
    params->b = 1;

    u->params = (void *)params;
    u->process = process_mult_ugen;
    u->destroy = destroy_mult_ugen;
    u->get_output = get_mult_ugen_output;
    u->set_param = set_mult_ugen_param;

    return u;
}

