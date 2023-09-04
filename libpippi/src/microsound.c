#include "microsound.h"

lpgrain_t * grain_create(lpfloat_t length, lpbuffer_t * window, lpbuffer_t * source) {
    lpgrain_t * g;

    g = (lpgrain_t *)LPMemoryPool.alloc(1, sizeof(lpgrain_t));
    g->speed = 1.f;
    g->pulsewidth = 1.f;
    g->length = length;
    g->range = length;
    g->buf = source;
    g->start = 0;
    g->offset = 0;

    g->phase = 0.f;
    g->phase_offset = LPRand.rand(0.f, 1.f);

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
    size_t idxa, idxb;

    ipw = 1.f;
    if(g->pulsewidth > 0) ipw = 1.0f/g->pulsewidth;

    phase = g->phase * ipw + g->start;
    channels = g->buf->channels;

    f = phase - (int)phase;
    idxa = (size_t)phase;
    idxb = idxa + 1;

    window_phase = (g->phase / g->length)  * ipw;
    while(window_phase >= 1.f) window_phase -= 1.f;
    window_phase *= g->window->length;

    if(ipw == 0.f || phase >= g->buf->length - 1) {
        for(c=0; c < channels; c++) {
            g->current_frame->data[c] = 0.f;
        }
    } else {
        for(c=0; c < channels; c++) {
            a = g->buf->data[idxa * channels + c];
            b = g->buf->data[idxb * channels + c];
            sample = (1.f - f) * a + (f * b);
            sample *= LPInterpolation.linear(g->window, window_phase);
            g->current_frame->data[c] = sample;

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
    }

    g->phase += g->speed;

    if(g->phase >= g->length) {
        g->phase -= g->length;
        g->gate = 1;
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

lpformation_t * formation_create(int window_type, int numgrains, size_t maxgrainlength, size_t mingrainlength, size_t rblength, int channels, int samplerate, lpbuffer_t * user_window) {
    lpformation_t * formation;
    size_t i;
    lpfloat_t grainlength;

    formation = (lpformation_t *)LPMemoryPool.alloc(1, sizeof(lpformation_t));
    if(window_type == WIN_USER) {
        formation->window = user_window;
    } else {
        formation->window = LPWindow.create(window_type, 4096);
    }

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
    formation->pos = 0.f;

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
    size_t j, grainlength;
    lpfloat_t pan;

    for(i=0; i < c->current_frame->channels; i++) {
        c->current_frame->data[i] = 0.f;
    }

    for(j=0; j < c->numgrains; j++) {
        if(c->grains[j]->gate == 1) {
            grainlength = (size_t)LPRand.rand(c->minlength, c->maxlength);
            c->grains[j]->skew = c->skew;
            c->grains[j]->speed = c->speed;
            c->grains[j]->length = grainlength;
            c->grains[j]->range = grainlength;
            c->grains[j]->start = (size_t)(c->rb->length * c->pos);
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

/**
 * Waveset segmentation tools
 */
int extract_wavesets(
    int num_wavesets,
    int num_crossings,
    lpfloat_t * source_buffer, 
    size_t source_buffer_length, 
    lpfloat_t * waveset_buffer, 
    size_t waveset_buffer_length,
    size_t * waveset_onsets,
    size_t * waveset_lengths
) {
    size_t i, j, lastonset;
    int current, lastsign, crossing_count, waveset_count;
    lpfloat_t input;

    input = 0.f;
    lastsign = 0;
    lastonset = 0;
    crossing_count = 0;
    waveset_count = 0;

    for(i=0; i < source_buffer_length; i++) {
        if(j >= waveset_buffer_length) break;
        if(waveset_count >= num_wavesets) break;

        input = source_buffer[i];

        current = signbit(input); 
        if((lastsign && !current) || (!lastsign && current)) {
            crossing_count += 1;

            if(crossing_count >= num_crossings) {
                waveset_lengths[waveset_count] = lastonset - j;
                waveset_onsets[waveset_count] = j;
                lastonset = j;

                waveset_count += 1;
                crossing_count = 0;
            }
        }

        lastsign = current;
        waveset_buffer[j] = input;

        j += 1;
    }

    return 0;
}



const lpgrain_factory_t LPGrain = { grain_create, grain_process, grain_destroy };
const lpformation_factory_t LPFormation = { formation_create, formation_process, formation_destroy };


