#include "fx.softclip.h"

/* Zener diode clipping simulation ported from 
 * Will Mitchell's pippi cython implementation.
 */

lpfxsoftclip_t * lpfxsoftclip_create(void);
lpfloat_t lpfxsoftclip_blsc_integrated_clip(lpfloat_t val);
lpfloat_t lpfxsoftclip_process(lpfxsoftclip_t * sc, lpfloat_t val);
void lpfxsoftclip_destroy(lpfxsoftclip_t * sc);

const lpfxsoftclip_factory_t LPSoftClip = { lpfxsoftclip_create, lpfxsoftclip_process, lpfxsoftclip_destroy };

lpfxsoftclip_t * lpfxsoftclip_create(void) {
    lpfxsoftclip_t * sc = (lpfxsoftclip_t *)LPMemoryPool.alloc(1, sizeof(lpfxsoftclip_t));
    sc->lastval = 0.f;
    return sc;
}

lpfloat_t lpfxsoftclip_blsc_integrated_clip(lpfloat_t val) {
    lpfloat_t out;

    if(val < -1) {
        out = -4/5.f * val - (1/3.f);
    } else {
        out = (val * val) / 2.f - lpfpow(val, 6) / 30.f;
    }

    if(val < 1) {
        return out;
    } else {
        return 4.f/5.f * val - 1.f/3.f;
    }
}

lpfloat_t lpfxsoftclip_process(lpfxsoftclip_t * sc, lpfloat_t val) {
    lpfloat_t sample = 0.f;

    if(lpfabs(val - sc->lastval) == 0.f) {
        sample = (val + sc->lastval) / 2.f;

        if(sample < -1) {
            sample = -4.f / 5.f;
        } else {
            sample = sample - lpfpow(sample, 5) / 5.f;
        }

        if(sample >= 1) {
            sample = 4.f / 5.f;
        }
    } else {
        sample = (lpfxsoftclip_blsc_integrated_clip(val) - lpfxsoftclip_blsc_integrated_clip(sc->lastval)) / (val - sc->lastval);
    }

    return sample;
}

void lpfxsoftclip_destroy(lpfxsoftclip_t * sc) {
    LPMemoryPool.free(sc);
}
