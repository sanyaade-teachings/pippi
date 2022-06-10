#ifndef LPASTRID
#define LPASTRID

int lprendernode_init();

typedef struct lpastridctx_t {
    lpscheduler_t * s;
    int channels;
    float samplerate;
    int blocksize;
} lpastridctx_t;

#endif
