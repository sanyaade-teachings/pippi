#include "microsound.h"

void grain_process(lpgrain_t * g, lpbuffer_t * out) {
    int c, oc=0;
    lpfloat_t win=0.f;

    assert(!isnan(g->src->phase));
    assert(g->samplerate > 0);
    assert(g->grainlength > 0);
    assert(out->length == 1); // outbuf is just a single frame vector

    if(g->pulsewidth <= 0) return;

    g->src->range = g->grainlength * g->samplerate;
    g->src->start = g->offset * g->samplerate;
    g->win->phase = g->src->phase; 
    g->src->pulsewidth = g->pulsewidth; 
    g->win->pulsewidth = g->pulsewidth; 

    LPTapeOsc.process(g->src); 
    LPTapeOsc.process(g->win); 

    g->gate = g->src->gate;

    /* window sources always get mixed to mono */
    for(c=0; c < g->win->buf->channels; c++) {
        win += g->win->current_frame->data[c];
    }

    /* grain sources get mapped to the grain output channels */
    for(c=0; c < out->channels; c++) {
        oc = c; 
        while(oc >= g->src->current_frame->channels) oc -= g->src->current_frame->channels;
        out->data[c] += g->src->current_frame->data[oc] * win;
    }
}

void grain_init(lpgrain_t * grain, lpbuffer_t * src, lpbuffer_t * win) {
    win->samplerate = src->samplerate;
    grain->src = LPTapeOsc.create(src);
    grain->win = LPTapeOsc.create(win);
    grain->offset = 0.f;
    grain->channels = src->channels;
    grain->samplerate = src->samplerate;
    grain->pulsewidth = 1.f;
}

lpformation_t * formation_create(int numgrains, lpbuffer_t * src, lpbuffer_t * win) {
    lpformation_t * f;
    lpfloat_t phaseinc, grainlength=.3f, pulsewidth=1.f,
              offset=0.f, speed=1.f, pan=0.5f;
    int g;

    assert(src->samplerate > 0);
    assert(src->length > 0);
    assert(src->channels > 0);
    assert(numgrains < LPFORMATION_MAXGRAINS);

    f = (lpformation_t *)LPMemoryPool.alloc(1, sizeof(lpformation_t));
    f->current_frame = LPBuffer.create(1, src->channels, src->samplerate);
    f->numgrains = numgrains;
    f->grainlength = grainlength;
    f->pulsewidth = pulsewidth;
    f->offset = offset;
    f->speed = speed;
    f->pan = pan;
    
    phaseinc = 1.f/numgrains;
    for(g=0; g < numgrains; g++) {
        grain_init(&f->grains[g], src, win);
        f->grains[g].grainlength = grainlength;
        f->grains[g].offset = offset;
        f->grains[g].pulsewidth = pulsewidth;
        f->grains[g].pan = pan;
        f->grains[g].src->speed = speed;
        f->grains[g].src->phase = g*phaseinc;
        f->grains[g].win->phase = f->grains[g].src->phase;
    }

    return f;
}

void formation_process(lpformation_t * f) {
    int g;

    memset(f->current_frame->data, 0, sizeof(lpfloat_t) * f->current_frame->channels);

    for(g=0; g < f->numgrains; g++) {
        grain_process(&f->grains[g], f->current_frame);
        if(f->grains[g].gate) {
            f->grains[g].pulsewidth = f->pulsewidth;
            f->grains[g].src->speed = f->speed;

            if(f->spread > 0) {
                f->grains[g].pan = .5f + LPRand.rand(-.5f, .5f) * f->spread;
            } else {
                f->grains[g].pan = f->pan;
            }

            if(f->grainlength_jitter > 0) {
                f->grains[g].grainlength = f->grainlength + (size_t)LPRand.rand(0, f->grainlength_jitter * f->grainlength_maxjitter);
            } else {
                f->grains[g].grainlength = f->grainlength;
            }

            if(f->grid_jitter > 0) {
                f->grains[g].offset = f->offset + (size_t)LPRand.rand(0, f->grid_jitter * f->grid_maxjitter);
            } else {
                f->grains[g].offset = f->offset;
            }

        }
    }

    while(f->offset >= f->grains[0].src->buf->length) f->offset -= f->grains[0].src->buf->length;
}

void formation_destroy(lpformation_t * c) {
    LPBuffer.destroy(c->window);
    LPBuffer.destroy(c->source);
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
    j = 0;

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

const lpformation_factory_t LPFormation = { formation_create, formation_process, formation_destroy };
