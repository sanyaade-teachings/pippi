#ifndef LP_UGEN_TAPE_H
#define LP_UGEN_TAPE_H

#include "oscs.tape.h"

#define UGEN_TAPE_PARAM_FREQ "freq"
#define UGEN_TAPE_PARAM_PHASE "phase"

enum UgenTapeParams {
    UTAPEIN_SPEED,
    UTAPEIN_PHASE,
    UTAPEIN_BUF,
    UTAPEIN_PULSEWIDTH,
    UTAPEIN_START,
    UTAPEIN_START_INCREMENT,
    UTAPEIN_RANGE,
};

/*
u_int32_t UgenTapeParamHashmap[7] = {
    LPHASHSTR("speed"),
    LPHASHSTR("phase"),
    LPHASHSTR("buf"),
    LPHASHSTR("pulsewidth"),
    LPHASHSTR("start"),
    LPHASHSTR("startinc"),
    LPHASHSTR("range"),
};
*/

enum UgenTapeOutputs {
    UTAPEOUT_MAIN,
    UTAPEOUT_SPEED,
    UTAPEOUT_PHASE,
    UTAPEOUT_GATE,
};

/*
u_int32_t UgenTapeParamHashmap[4] = {
    LPHASHSTR("main"),
    LPHASHSTR("speed"),
    LPHASHSTR("phase"),
    LPHASHSTR("gate"),
};
*/

typedef struct lpugentape_t {
    lptapeosc_t * osc;
    lpfloat_t outputs[4];
} lpugentape_t;

ugen_t * create_tape_ugen(void);

#endif
