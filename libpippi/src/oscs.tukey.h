#ifndef LP_TUKEYOSC_H
#define LP_TUKEYOSC_H

#include "pippicore.h"

typedef struct lptukeyosc_t {
    lpfloat_t phase;
    lpfloat_t freq;
    lpfloat_t shape;
    int direction;
    lpfloat_t samplerate;
} lptukeyosc_t;

typedef struct lptukeyosc_factory_t {
    lptukeyosc_t * (*create)(void);
    lpfloat_t (*process)(lptukeyosc_t *);
    lpbuffer_t * (*render)(lptukeyosc_t*, size_t, lpbuffer_t *, lpbuffer_t *, int);
    void (*destroy)(lptukeyosc_t *);
} lptukeyosc_factory_t;

extern const lptukeyosc_factory_t LPTukeyOsc;

#endif
