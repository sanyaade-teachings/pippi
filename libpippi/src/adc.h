#ifndef LPADC
#define LPADC

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#include "pippi.h"
#include <hiredis/hiredis.h>

typedef struct lpadcctx_t {
    lpbuffer_t * adc;
    char * framestr;
    int blocksize;
    redisContext * c;
    redisReply * r;
} lpadcctx_t;

#endif
