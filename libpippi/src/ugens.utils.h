#ifndef LP_UGEN_MULT_H
#define LP_UGEN_MULT_H

#define UGEN_MULT_PARAM_A "a"
#define UGEN_MULT_PARAM_B "b"

enum UgenUtilsParams {
    UMULTIN_A,
    UMULTIN_B
};

enum UgenUtilsOutputs {
    UMULTOUT_MAIN,
    UMULTOUT_A,
    UMULTOUT_B
};

typedef struct lpugenmult_t {
    lpfloat_t a;
    lpfloat_t b;
    lpfloat_t outputs[3];
} lpugenmult_t;

ugen_t * create_mult_ugen(void);

#endif
