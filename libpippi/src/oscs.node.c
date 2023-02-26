#include "oscs.node.h"

lpnode_t * lpnode_create(int node_type);
void lpnode_connect(lpnode_t * node, int param_type, lpnode_t * param, lpfloat_t minval, lpfloat_t maxval);
void lpnode_connect_signal(lpnode_t * node, int param_type, lpfloat_t value);
lpfloat_t lpnode_process(lpnode_t * node);
void lpnode_destroy(lpnode_t * node);

const lpnode_factory_t LPNode = { lpnode_create, lpnode_connect, lpnode_connect_signal, lpnode_process, lpnode_destroy };

lpnode_t * lpnode_create(int node_type) {
    lpnode_t * node;

    // create the node struct
    node = (lpnode_t *)LPMemoryPool.alloc(1, sizeof(lpnode_t));
    node->type = node_type;
    node->last = 0.f;

    // create the param struct in the appropriate flavor
    if(node_type == NODE_SIGNAL) {
        node->params.signal = (lpnode_signal_t *)LPMemoryPool.alloc(1, sizeof(lpnode_signal_t));
    } else if(node_type == NODE_SINEOSC) {
        node->params.sineosc = (lpnode_sineosc_t *)LPMemoryPool.alloc(1, sizeof(lpnode_sineosc_t));
        node->params.sineosc->freq_mul = 1.f;
        node->params.sineosc->freq_add = 0.f;
        node->params.sineosc->phase = 0.f;
        node->params.sineosc->samplerate = DEFAULT_SAMPLERATE;
    }

    return node;
}

void lpnode_connect(lpnode_t * node, int param_type, lpnode_t * param, lpfloat_t minval, lpfloat_t maxval) {
    if(node->type == NODE_SINEOSC) {
        if(param_type == PARAM_FREQ) {
            node->params.sineosc->freq = param;
            node->params.sineosc->freq_mul = (maxval - minval) / 2.f;
            node->params.sineosc->freq_add = minval + node->params.sineosc->freq_mul;
        }
    }
}

void lpnode_connect_signal(lpnode_t * node, int param_type, lpfloat_t value) {
    if(node->type == NODE_SINEOSC) {
        if(param_type == PARAM_FREQ) {
            node->params.sineosc->freq = lpnode_create(NODE_SIGNAL);
            node->params.sineosc->freq->last = value;
            node->params.sineosc->freq->params.signal->value = value;
            node->params.sineosc->freq_mul = 1.f;
            node->params.sineosc->freq_add = 0.f;
        }
    }
}


lpfloat_t lpnode_sineosc_process(lpnode_t * node) {
    lpfloat_t sample, freq;
    
    sample = sin((lpfloat_t)PI2 * node->params.sineosc->phase);
    freq = node->params.sineosc->freq->last;
    freq *= node->params.sineosc->freq_mul;
    freq += node->params.sineosc->freq_add;
    node->params.sineosc->phase += freq * (1.0f/node->params.sineosc->samplerate);

    while(node->params.sineosc->phase >= 1) {
        node->params.sineosc->phase -= 1.0f;
    }

    node->last = sample;
    return sample;
}

lpfloat_t lpnode_process(lpnode_t * node) {
    if(node->type == NODE_SIGNAL) {
        return node->params.signal->value;
    } else if(node->type == NODE_SINEOSC) {
        return lpnode_sineosc_process(node);        
    }

    return 0;
}

void lpnode_destroy(lpnode_t * node) {
    LPMemoryPool.free(node);
}
