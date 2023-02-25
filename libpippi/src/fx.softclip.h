#ifndef LP_FXSOFTCLIP
#define LP_FXSOFTCLIP

#include "pippicore.h"

typedef struct lpfxsoftclip_t {
    lpfloat_t lastval;
} lpfxsoftclip_t;

typedef struct lpfxsoftclip_factory_t {
    lpfxsoftclip_t * (*create)(void);
    lpfloat_t (*process)(lpfxsoftclip_t * sc, lpfloat_t val);
    void (*destroy)(lpfxsoftclip_t * sc);
} lpfxsoftclip_factory_t;

extern const lpfxsoftclip_factory_t LPSoftClip;

#endif
