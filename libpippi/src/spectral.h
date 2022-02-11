#ifndef LP_SPECTRAL_H
#define LP_SPECTRAL_H

#include "pippicore.h"
#include "fft/fft.h"

typedef struct lpspectral_factory_t {
    lpbuffer_t * (*convolve)(lpbuffer_t *, lpbuffer_t *);
} lpspectral_factory_t;

extern const lpspectral_factory_t LPSpectral;

#endif
