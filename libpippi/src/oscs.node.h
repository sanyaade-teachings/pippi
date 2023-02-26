#ifndef LP_NODEOSC_H
#define LP_NODEOSC_H

#include "pippicore.h"

typedef struct lpnode_t lpnode_t;

enum NodeTypes {
    NODE_TABLE,
    NODE_SIGNAL,
    NODE_SINEOSC
};

enum ParamTypes {
    PARAM_FREQ,
    PARAM_AMP
};

typedef struct lpnode_table_t {
    lpbuffer_t * buf;
    lpfloat_t * freq;
} lpnode_table_t;

typedef struct lpnode_signal_t {
    lpfloat_t value;
} lpnode_signal_t;

typedef struct lpnode_sineosc_t {
    lpnode_t * freq;
    lpfloat_t freq_mul;
    lpfloat_t freq_add;
    lpfloat_t phase;
    lpfloat_t samplerate;
} lpnode_sineosc_t;

typedef struct lpnode_t {
    int type;
    lpfloat_t last;
    union params {
        lpnode_table_t * table;
        lpnode_signal_t * signal;
        lpnode_sineosc_t * sineosc;
    } params;
} lpnode_t;

typedef struct lpnode_factory_t {
    lpnode_t * (*create)(int node_type);
    void (*connect)(lpnode_t * node, int param_type, lpnode_t * param, lpfloat_t minval, lpfloat_t maxval);
    void (*connect_signal)(lpnode_t * node, int param_type, lpfloat_t value);
    lpfloat_t (*process)(lpnode_t * node);
    void (*destroy)(lpnode_t * node);
} lpnode_factory_t;

extern const lpnode_factory_t LPNode;

#endif
