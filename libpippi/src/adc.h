#ifndef LPADC
#define LPADC

typedef struct lpadcctx_t {
    lpbuffer_t * adc;
    char * framestr;
    int blocksize;
    redisContext * c;
    redisReply * r;
} lpadcctx_t;

#endif
