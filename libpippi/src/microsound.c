#include "microsound.h"

lpgrain_t * grain_create(lpfloat_t length, lpbuffer_t * window, lpbuffer_t * source) {
    lpgrain_t * g;

    g = (lpgrain_t *)LPMemoryPool.alloc(1, sizeof(lpgrain_t));
    g->speed = 1.f;
    g->length = length;
    g->offset = 0.f;
    g->osc = LPTapeOsc.create(source, length);

    g->pan = 0.5f;
    g->amp = 1.f;
    g->jitter = 0.f;

    g->window = window;

    return g;
}

void grain_process(lpgrain_t * g, lpbuffer_t * out) {
    lpfloat_t sample;
    int is_odd, c;
    lpfloat_t window_phase;

    LPTapeOsc.process(g->osc);
    window_phase = (g->osc->phase / g->osc->range) * g->window->length;

    for(c=0; c < out->channels; c++) {
        sample = g->osc->current_frame->data[c] * LPFX.read_skewed_buffer(1.f, g->window, window_phase, g->skew);

        if(g->pan != 0.5f) {
            is_odd = (2-(c & 1));
            if(is_odd == 1) {
                sample *= (1.f - g->pan);
            } else {
                sample *= g->pan;
            }
        }

        out->data[c] += sample * g->amp;
    }

    if(g->osc->gate == 1) {
        g->osc->range = g->length;
        g->osc->speed = g->speed;
    }

}

void grain_destroy(lpgrain_t * g) {
    if(g != NULL) LPTapeOsc.destroy(g->osc);
    LPMemoryPool.free(g);
}

lpformation_t * formation_create(int numstreams, size_t maxgrainlength, size_t mingrainlength, size_t rblength, int channels, int samplerate) {
    lpformation_t * formation;
    size_t i;
    lpfloat_t grainlength;

    formation = (lpformation_t *)LPMemoryPool.alloc(1, sizeof(lpformation_t));
    formation->window = LPWindow.create(WT_HANN, 4096);
    formation->rb = LPRingBuffer.create(rblength, channels, samplerate);
    formation->maxlength = maxgrainlength;
    formation->minlength = mingrainlength;
    formation->numgrains = numstreams * 2;
    formation->grains = (lpgrain_t **)LPMemoryPool.alloc(formation->numgrains, sizeof(lpgrain_t *));
    formation->grainamp = 1.f;
    formation->current_frame = LPBuffer.create(1, channels, samplerate);
    formation->speed = 1.f;
    formation->scrub = 1.f;
    formation->spread = 0.f;
    formation->skew = 0.f;

    for(i=0; i < formation->numgrains; i += 2) {
        grainlength = LPRand.rand(formation->minlength, formation->maxlength);
        formation->grains[i] = grain_create(grainlength, formation->window, formation->rb);
        formation->grains[i]->amp = formation->grainamp;
        formation->grains[i+1] = grain_create(grainlength, formation->window, formation->rb);
        formation->grains[i+1]->amp = formation->grainamp;
    }

    return formation;
}

void formation_process(lpformation_t * c) {
    int i;
    size_t j;
    lpfloat_t grainlength, pan;

    for(i=0; i < c->current_frame->channels; i++) {
        c->current_frame->data[i] = 0.f;
    }

    for(j=0; j < c->numgrains; j += 2) {
        grainlength = LPRand.rand(c->minlength, c->maxlength);

        c->grains[j]->skew = c->skew;
        c->grains[j]->speed = c->speed;
        c->grains[j]->length = grainlength;
        grain_process(c->grains[j], c->current_frame);

        c->grains[j+1]->skew = c->skew;
        c->grains[j+1]->speed = c->speed;
        c->grains[j+1]->length = grainlength;
        grain_process(c->grains[j+1], c->current_frame);

        if(c->grains[j]->osc->gate > 0) {
            c->grains[j]->osc->offset += grainlength * c->scrub;
            if(c->grains[j]->osc->offset > c->grains[j]->osc->buf->length) {
                c->grains[j]->osc->offset -= c->grains[j]->osc->buf->length;
            }
        }

        if(c->grains[j+1]->osc->gate > 0) {
            c->grains[j+1]->osc->offset += grainlength * c->scrub;
            if(c->grains[j+1]->osc->offset > c->grains[j]->osc->buf->length) {
                c->grains[j+1]->osc->offset -= c->grains[j]->osc->buf->length;
            }
        }

        if(c->spread > 0) {
            pan = 0.5f + LPRand.rand(-.5f, 0.5f) * c->spread;
            c->grains[j]->pan = pan;
            c->grains[j+1]->pan = pan;
        }
    }
}

void formation_destroy(lpformation_t * c) {
    size_t i;
    for(i=0; i < c->numgrains; i++) {
        LPMemoryPool.free(c->grains[i]);
    }
    LPMemoryPool.free(c->grains);
    LPBuffer.destroy(c->window);
    LPBuffer.destroy(c->rb);
    LPBuffer.destroy(c->current_frame);
    LPMemoryPool.free(c);
}

const lpgrain_factory_t LPGrain = { grain_create, grain_process, grain_destroy };
const lpformation_factory_t LPFormation = { formation_create, formation_process, formation_destroy };


