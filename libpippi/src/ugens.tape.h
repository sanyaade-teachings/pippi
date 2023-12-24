#ifndef LP_UGEN_TAPE_H
#define LP_UGEN_TAPE_H

#include "oscs.tape.h"

#define UGEN_TAPE_PARAM_FREQ "freq"
#define UGEN_TAPE_PARAM_PHASE "phase"

enum UgenTapeParams {
    UTAPEIN_SPEED,
    UTAPEIN_PHASE,
    UTAPEIN_BUF
};

enum UgenTapeOutputs {
    UTAPEOUT_MAIN,
    UTAPEOUT_SPEED,
    UTAPEOUT_PHASE
};

typedef struct lpugentape_t {
    lptapeosc_t * osc;
    lpfloat_t outputs[3];
} lpugentape_t;

ugen_t * create_tape_ugen(void);

#endif
