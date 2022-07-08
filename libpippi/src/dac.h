#ifndef LPDAC_H
#define LPDAC_H

typedef struct lpdacctx_t {
    lpscheduler_t * s;
    int channels;
    float samplerate;
} lpdacctx_t;

#endif
