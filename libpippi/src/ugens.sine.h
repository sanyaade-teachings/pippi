#ifndef LP_UGEN_SINE_H
#define LP_UGEN_SINE_H

#include "oscs.sine.h"

#define UGEN_SINE_PARAM_FREQ "freq"
#define UGEN_SINE_PARAM_PHASE "phase"

enum UgenSineParams {
    USINEIN_FREQ,
    USINEIN_PHASE
};

enum UgenSineOutputs {
    USINEOUT_MAIN,
    USINEOUT_FREQ,
    USINEOUT_PHASE
};

typedef struct lpugensine_t {
    lpsineosc_t * osc;
    lpfloat_t outputs[3];
} lpugensine_t;


ugen_t * create_sine_ugen(void);

#endif
