#include "microsound.h"

lpgrain_t * grain_create(lpfloat_t length, lpbuffer_t * window, lpbuffer_t * source) {
    lpgrain_t * g;

    g = (lpgrain_t *)LPMemoryPool.alloc(1, sizeof(lpgrain_t));
    g->speed = 1.f;
    g->pulsewidth = 1.f;
    g->length = length;
    g->buf = source;
    g->start = 0;
    g->offset = 0;

    g->phase = LPRand.rand(0.f, 1000.f);
    g->pan = 0.5f;
    g->amp = 1.f;
    g->gate = 0;
    g->skew = 0.f;

    g->window = window;
    g->current_frame = LPBuffer.create(1, source->channels, source->samplerate);

    return g;
}

void grain_process(lpgrain_t * g, lpbuffer_t * out) {
    lpfloat_t sample, f, a, b, ipw, phase, window_phase;
    int c, channels, is_odd;
    size_t idxa, idxb, boundry;

    ipw = 1.f;
    if(g->pulsewidth > 0) ipw = 1.0f/g->pulsewidth;

    phase = (g->phase + g->offset) * ipw;
    channels = g->buf->channels;
    boundry = g->range + g->start;

    f = phase - (int)phase;
    idxa = (size_t)phase;
    idxb = idxa + 1;

    if(ipw == 0.f || phase >= g->buf->length - 1) {
        for(c=0; c < channels; c++) {
            g->current_frame->data[c] = 0.f;
        }
    } else {
        for(c=0; c < channels; c++) {
            a = g->buf->data[idxa * channels + c];
            b = g->buf->data[idxb * channels + c];
            sample = (1.f - f) * a + (f * b);
            g->current_frame->data[c] = sample;
        }
    }

    window_phase = ((g->phase - g->start + g->offset) / g->range) * g->window->length;

    for(c=0; c < out->channels; c++) {
        /*sample = g->osc->current_frame->data[c] * LPFX.read_skewed_buffer(1.f, g->window, window_phase, g->skew);*/
        sample = g->current_frame->data[c] * LPInterpolation.linear(g->window, window_phase);

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

    g->phase += g->speed;

    if(g->phase + g->offset >= boundry) {
        /*printf("Tapeosc phase reset! Phase: %f Length: %f boundry: %f idxb: %f\n", osc->phase, (float)osc->buf->length, (float)boundry, (float)idxb);*/
        /*osc->start += osc->start_increment;*/
        g->phase = g->start;
        g->gate = 1;

        g->start += g->range;
        g->range = g->length;

        if(g->start > g->buf->length + g->range) {
            /*printf("reset! start: %d length: %d\n", (int)g->start, (int)g->buf->length);*/
            g->phase = g->offset;
            g->start = g->offset;
        }
    } else {
        g->gate = 0;
    }
}

void grain_destroy(lpgrain_t * g) {
    if(g != NULL) {
        LPBuffer.destroy(g->current_frame);
        LPMemoryPool.free(g);
    }
}

lpformation_t * formation_create(int numgrains, size_t maxgrainlength, size_t mingrainlength, size_t rblength, int channels, int samplerate) {
    lpformation_t * formation;
    size_t i;
    lpfloat_t grainlength;

    formation = (lpformation_t *)LPMemoryPool.alloc(1, sizeof(lpformation_t));
    formation->window = LPWindow.create(WIN_HANN, 4096);
    formation->rb = LPRingBuffer.create(rblength, channels, samplerate);
    formation->maxlength = maxgrainlength;
    formation->minlength = mingrainlength;
    formation->numgrains = numgrains;
    formation->grains = (lpgrain_t **)LPMemoryPool.alloc(formation->numgrains, sizeof(lpgrain_t *));
    formation->grainamp = 1.f;
    formation->current_frame = LPBuffer.create(1, channels, samplerate);
    formation->speed = 1.f;
    formation->scrub = 1.f;
    formation->spread = 0.f;
    formation->skew = 0.f;
    formation->pulsewidth = 1.f;

    for(i=0; i < formation->numgrains; i++) {
        grainlength = LPRand.rand(formation->minlength, formation->maxlength);
        formation->grains[i] = grain_create(grainlength, formation->window, formation->rb);
        formation->grains[i]->amp = formation->grainamp;
        formation->grains[i]->pulsewidth = formation->pulsewidth;
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

    for(j=0; j < c->numgrains; j++) {
        if(c->grains[j]->gate == 1) {
            grainlength = LPRand.rand(c->minlength, c->maxlength);
            c->grains[j]->skew = c->skew;
            c->grains[j]->speed = c->speed;
            c->grains[j]->length = grainlength;
            c->grains[j]->range = (size_t)grainlength;
            if(c->spread > 0) {
                pan = 0.5f + LPRand.rand(-.5f, 0.5f) * c->spread;
                c->grains[j]->pan = pan;
            }
        }
        grain_process(c->grains[j], c->current_frame);
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


