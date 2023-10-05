#include "microsound.h"

void grain_process(lpgrain_t * g, lpbuffer_t * out) {
    lpfloat_t sample, ipw, phase, window_phase;
    int c;

    ipw = 1.f;
    if(g->pulsewidth > 0) ipw = 1.0f/g->pulsewidth;

    phase = g->phase * ipw + g->start;

    window_phase = (g->phase / g->length)  * ipw;
    while(window_phase >= 1.f) window_phase -= 1.f;
    window_phase *= g->window->length;

    if(ipw > 0.f && phase < g->buf->length - 1) {
        for(c=0; c < g->buf->channels; c++) {
            sample = LPInterpolation.linear_channel(g->buf, phase, c);
            sample *= LPInterpolation.linear(g->window, window_phase);

            if(g->pan != 0.5f) {
                if((2-(c & 1)) == 1) { /* checks if c is odd */
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
        /* recycle this grain, we're done with it now */
        g->phase = 0;
        g->in_use = 0; 
    }
}

lpformation_t * formation_create(int window_type, int numlayers, size_t grainlength, size_t rblength, int channels, int samplerate, lpbuffer_t * user_window) {
    lpformation_t * formation;

    formation = (lpformation_t *)LPMemoryPool.alloc(1, sizeof(lpformation_t));
    if(window_type == WIN_USER) {
        formation->window = user_window;
    } else {
        formation->window = LPWindow.create(window_type, 4096);
    }

    formation->rb = LPRingBuffer.create(rblength, channels, samplerate);
    formation->grainlength = grainlength;
    formation->numlayers = numlayers;
    formation->amp = 1.f;
    formation->current_frame = LPBuffer.create(1, channels, samplerate);
    formation->speed = 1.f;
    formation->scrub = 1.f;
    formation->spread = 0.f;
    formation->skew = 0.f;
    formation->pulsewidth = 1.f;
    formation->pos = 0.f;
    formation->pan = 0.5f;

    formation->num_active_grains = 0;

    formation->phase = 0.f;
    formation->phase_inc = 1.f / samplerate;

    formation->onset_phase = 0.f;
    formation->onset_phase_inc = 1.f / rblength;

    formation->graininterval = grainlength / 2;

    return formation;
}

void formation_process(lpformation_t * c) {
    int i, g, gi, active_grains, activated_grains, requested_grains, new_grain;
    size_t grainlength;
    lpfloat_t pan;
    int grain_indexes[LPFORMATION_MAXGRAINS];

    for(i=0; i < c->current_frame->channels; i++) {
        c->current_frame->data[i] = 0.f;
    }

    /* calculate the grid phase */

    /* if the grid phase has reset, add some grains to the stack 
     * at this point, formation params are taken as a snapshot and 
     * copied into the new grains.
     * */
    requested_grains = 0;
    if(c->onset_phase >= 1.f) {
        requested_grains = c->numlayers;
    }

    /* adding a grain to the stack: */
    /* reset grain indexes, loop over to find active indexes, 
     * use first open indexes for new grains */
    active_grains = 0;
    activated_grains = 0;
    g = 0;
    gi = 0;

    if(requested_grains == 0 && c->num_active_grains == 0) {
        requested_grains += 1;
    }

    while(1) {
        if(active_grains == 0 && requested_grains == 0) break;

        new_grain = 0;
        if(c->grains[g].in_use == 0 && activated_grains < requested_grains) {
            if(c->grainlength_jitter > 0) {
                grainlength = c->grainlength + (size_t)LPRand.rand(0, c->grainlength_jitter * c->grainlength_maxjitter);
            } else {
                grainlength = c->grainlength;
            }

            /* add a new grain to the stack by reusing this one */
            c->grains[g].in_use = 1;
            c->grains[g].phase = 0;

            c->grains[g].buf = c->rb;
            c->grains[g].offset = c->offset / (lpfloat_t)c->rb->samplerate;
            c->grains[g].pulsewidth = c->pulsewidth;

            c->grains[g].phase = 0.f;
            c->grains[g].phase_offset = LPRand.rand(0.f, 1.f);

            c->grains[g].amp = c->amp;

            c->grains[g].window = c->window;

            c->grains[g].skew = c->skew;
            c->grains[g].speed = c->speed;

            c->grains[g].length = grainlength;
            c->grains[g].range = grainlength;
            c->grains[g].start = (size_t)(c->rb->length * c->pos);

            c->grains[g].pan = c->pan;
            if(c->spread > 0) {
                pan = 0.5f + LPRand.rand(-.5f, 0.5f) * c->spread;
                c->grains[g].pan = pan;
            }

            activated_grains += 1;
            new_grain = 1;
        }

        if(c->grains[g].in_use == 1) {
            grain_indexes[gi] = g;
            gi += 1;
            if(new_grain == 0) active_grains += 1;
        }

        g += 1;
        if(g >= LPFORMATION_MAXGRAINS) g -= LPFORMATION_MAXGRAINS;
        if(gi >= LPFORMATION_MAXGRAINS) break;
        if(active_grains + activated_grains >= c->num_active_grains + requested_grains) break;
    }

    /* process all the active grains into the output buffer */
    active_grains += activated_grains;
    c->num_active_grains = active_grains;
    for(gi=0; gi < active_grains; gi++) {
        grain_process(&c->grains[grain_indexes[gi]], c->current_frame);
    }

    /* increment the internal phases */
    c->phase += c->phase_inc;
    while(c->phase >= 1.f) c->phase -= 1.f;

    c->onset_phase += c->onset_phase_inc;
    while(c->onset_phase >= 1.f) c->onset_phase -= 1.f;

    c->pos += c->phase_inc;
    while(c->pos >= 1.f) c->pos -= 1.f;
}

void formation_destroy(lpformation_t * c) {
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


