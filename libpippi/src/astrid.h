#ifndef LPASTRID
#define LPASTRID

int lprendernode_init();

typedef struct lpastridctx_t {
    lpscheduler_t * s;
    lpbuffer_t * adc;
} lpastridctx_t;

#endif
