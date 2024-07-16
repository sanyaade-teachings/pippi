#include "pippi.h"
#include "ugens.tape.h"
#include "oscs.tape.h"


void process_tape_ugen(ugen_t * u) {
    lpugentape_t * params;
    lpfloat_t sample;
    params = (lpugentape_t *)u->params;
    LPTapeOsc.process(params->osc);

    sample = params->osc->current_frame->data[0];

    params->outputs[UTAPEOUT_MAIN] = sample;
    params->outputs[UTAPEOUT_SPEED] = params->osc->speed;
    params->outputs[UTAPEOUT_PHASE] = params->osc->phase;
}

void destroy_tape_ugen(ugen_t * u) {
    lpugentape_t * params;
    params = (lpugentape_t *)u->params;
    free(params->osc->current_frame);
    free(params->osc);
    free(params);
    free(u);
}

void set_tape_ugen_param(ugen_t * u, int index, void * value) {
    lpfloat_t * v;
    lpbuffer_t * buf;
    lpugentape_t * params;

    params = (lpugentape_t *)u->params;
    switch(index) {
        case UTAPEIN_SPEED:
            v = (lpfloat_t *)value;
            params->osc->speed = *v;
            break;

        case UTAPEIN_PHASE:
            v = (lpfloat_t *)value;
            params->osc->phase = *v;
            break;

        case UTAPEIN_BUF:
            buf = (lpbuffer_t *)value;
            printf("%i %d\n", buf->channels, buf->samplerate);
            params->osc->buf = buf;
            params->osc->samplerate = buf->samplerate;
            if(params->osc->current_frame != NULL) {
                free(params->osc->current_frame);
                params->osc->current_frame = LPBuffer.create(1, buf->channels, buf->samplerate);
            }
            params->osc->range = buf->length-1;
            break;

        case UTAPEIN_PULSEWIDTH:
            v = (lpfloat_t *)value;
            params->osc->pulsewidth = *v;
            break;

        case UTAPEIN_START:
            v = (lpfloat_t *)value;
            params->osc->start = *v;
            break;

        case UTAPEIN_RANGE:
            v = (lpfloat_t *)value;
            params->osc->range = *v;
            break;

        default:
            break;
    }
}

lpfloat_t get_tape_ugen_output(ugen_t * u, int index) {
    lpugentape_t * params;
    params = (lpugentape_t *)u->params;
    return params->outputs[index];
}

ugen_t * create_tape_ugen(void) {
    ugen_t * u;
    lpugentape_t * params;
    lpbuffer_t * frame = LPBuffer.create(1,1,1); // FIXME this leaks -- add req init params?

    params = (lpugentape_t *)LPMemoryPool.alloc(sizeof(lpugentape_t), 1);
    params->osc = LPTapeOsc.create(frame);
    u = (ugen_t *)LPMemoryPool.alloc(sizeof(ugen_t), 1);

    u->params = (void *)params;
    u->process = process_tape_ugen;
    u->destroy = destroy_tape_ugen;
    u->get_output = get_tape_ugen_output;
    u->set_param = set_tape_ugen_param;

    u->num_outlets = 4; // three param outlets
    u->num_inlets = 7; // seven param inlets
                      
    u->num_outputs = 1; // one audio output
    u->num_inputs = 0; // no audio inputs

    return u;
}

