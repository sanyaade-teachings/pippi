#include "mir.h"

/**
 * Yin implementation ported from:
 * Patrice Guyot. (2018, April 19). Fast Python 
 * implementation of the Yin algorithm (Version v1.1.1). 
 * Zenodo. http://doi.org/10.5281/zenodo.1220947
 * https://github.com/patriceguyot/Yin
 *
 * Coyote onset detector ported with permission from 
 * the SuperCollider BatUGens by Batuhan Bozkurt.
 * https://github.com/earslap/SCPlugins
 */

void yin_difference_function(lpyin_t * yin) {
    int j, tau, pos;
    lpfloat_t tmp, s1, s2;

    pos = yin->block->pos - 1;
    for(tau=1; tau < yin->tau_max; tau++) {
        for(j=0; j < yin->tau_max; j++) {
            s1 = yin->block->data[(pos + j) % yin->block->length];
            s2 = yin->block->data[(pos + j + tau) % yin->block->length];
            tmp = s1 - s2;
            yin->tmp->data[tau] += tmp * tmp;
        }
    }
}

void yin_cumulative_mean_normalized_difference_function(lpyin_t * yin) {
    lpfloat_t prev, denominator, value;
    int i;

    prev = 0;
    for(i=1; i < yin->tau_max; i++) {
        denominator = yin->tmp->data[i] + prev;
        value = (yin->tmp->data[i] * i) / denominator;
        prev = denominator;
        yin->tmp->data[i] = value;
    }
}

lpfloat_t yin_get_pitch(lpyin_t * yin) {
    int tau;
    tau = yin->tau_min;
    while (tau < yin->tau_max) {
        if(yin->tmp->data[tau] < yin->threshold) {
            while(tau + 1 < yin->tau_max && yin->tmp->data[tau + 1] < yin->tmp->data[tau]) {
                tau += 1;
            }
            return (lpfloat_t)yin->samplerate / tau;
        }
        tau += 1;
    }

    /* unvoiced */
    return yin->last_pitch;
}

lpfloat_t yin_process(lpyin_t * yin, lpfloat_t sample) {
    lpfloat_t p;
    yin->block->data[yin->block->pos] = sample;
    yin->block->pos += 1;
    yin->block->pos = yin->block->pos % yin->block->length;

    if(yin->elapsed >= yin->stepsize) {
        yin_difference_function(yin);
        yin_cumulative_mean_normalized_difference_function(yin);
        p = yin_get_pitch(yin);
        yin->last_pitch = p;
        yin->elapsed = 0;
    } else {
        p = yin->last_pitch;
    }

    yin->elapsed += 1;
    return p;
}

lpyin_t * yin_create(int blocksize, int samplerate) {
    lpfloat_t f0_max, f0_min;
    lpyin_t * yin;

    f0_max = 20000.f;
    f0_min = 100.f;

    yin = (lpyin_t *)LPMemoryPool.alloc(1, sizeof(lpyin_t));
    yin->samplerate = samplerate;
    yin->blocksize = blocksize;
    yin->stepsize = blocksize / 4;
    yin->tau_min = (int)(samplerate / f0_max);
    yin->tau_max = (int)(samplerate / f0_min);

    yin->block = LPBuffer.create(yin->blocksize, 1, samplerate);
    yin->tmp = LPBuffer.create(yin->tau_max, 1, samplerate);
    yin->fallback = 0.f; /* Fallback pitch in hz for output values before any pitch is detected */
    yin->last_pitch = yin->fallback;
    yin->threshold = 0.85f;
    yin->offset = 0;
    yin->elapsed = 0;

    return yin;
}

void yin_destroy(lpyin_t * yin) {
    LPMemoryPool.free(yin->tmp);
    LPMemoryPool.free(yin->block);
    LPMemoryPool.free(yin);
}

lpcoyote_t * coyote_create(int samplerate) {
    lpcoyote_t * od;

    od = (lpcoyote_t *)LPMemoryPool.alloc(1, sizeof(lpcoyote_t));

    od->log1 = log(0.1f);
    od->log01 = log(0.01f);
    od->log001 = log(0.001f);

	od->track_fall_time = 0.2f;
    od->slow_lag_time = 0.2f;
    od->fast_lag_time = 0.01f;
    od->fast_lag_mul = 0.5f;
    od->thresh = 0.05f;
    od->min_dur = 0.1f;

    od->rise_coef = exp(od->log1/(0.001 * samplerate));
    od->fall_coef = exp(od->log1 / (od->track_fall_time * od->samplerate));
    od->slow_lag_coef = exp(od->log001 / (od->slow_lag_time * samplerate));
    od->fast_lag_coef = exp(od->log001 / (od->fast_lag_time * samplerate));

    od->slow_lag_prev = 0.f;
    od->fast_lag_prev = 0.f;

    od->prev_amp = 0.f;
    od->avg_lag_prev = 0.f;

    od->avg_trig = 0.f;
    od->current_avg = 0.f;
    od->current_index = 1;

    od->e_time = 0;
    od->gate = 1;

    od->samplerate = samplerate;

    return od;
}

lpfloat_t coyote_process(lpcoyote_t * od, lpfloat_t sample) {
    lpfloat_t prev;
    lpfloat_t tracker_out, trig;
    lpfloat_t fast_val, slow_val;
    lpfloat_t avg_val, divi, out;

    od->fall_coef = exp(od->log1 / (od->track_fall_time * od->samplerate));
    od->slow_lag_coef = exp(od->log001 / (od->slow_lag_time * od->samplerate));
    od->fast_lag_coef = exp(od->log001 / (od->fast_lag_time * od->samplerate));

    prev = od->prev_amp;

    if(od->avg_trig) {
        od->current_avg = 0.f;
        od->current_index = 1;
    }

    /* Normally this would loop over a block of samples... */
    tracker_out = fabs(sample);
    if(tracker_out < prev) {
        tracker_out = tracker_out + (prev - tracker_out) * od->fall_coef;
    } else {
        tracker_out = tracker_out + (prev - tracker_out) * od->rise_coef;
    }

    divi = ((od->current_avg - tracker_out) / od->current_index);
    od->current_avg = od->current_avg - divi;
    od->current_index += 1;

    od->prev_amp = tracker_out;
    /* end loop */

    slow_val = od->slow_lag_prev = tracker_out + (od->slow_lag_coef * (od->slow_lag_prev - tracker_out));
    fast_val = od->fast_lag_prev = tracker_out + (od->fast_lag_coef * (od->fast_lag_prev - tracker_out));
    avg_val = od->avg_lag_prev = od->current_avg + (od->fast_lag_coef * (od->avg_lag_prev - od->current_avg));

    od->slow_lag_prev = lpzapgremlins(od->slow_lag_prev);
    od->fast_lag_prev = lpzapgremlins(od->fast_lag_prev);
    od->avg_lag_prev = lpzapgremlins(od->avg_lag_prev);

    trig = od->avg_trig = ((fast_val > slow_val) || (fast_val> avg_val)) * (tracker_out > od->thresh)  * od->gate;
    od->e_time += 1;

    out = trig;

    if((trig == 1) && (od->gate == 1)) {
        od->e_time = 0;
        od->gate = 0;
    }

    if((od->e_time > (od->samplerate * od->min_dur)) && (od->gate == 0)) {
        od->gate = 1;
    }

    return out;
}

void coyote_destroy(lpcoyote_t * od) {
    LPMemoryPool.free(od);
}

/**
 * Envelope follower
 */
lpenvelopefollower_t * envelopefollower_create(lpfloat_t interval) {
    lpenvelopefollower_t * env;

    env = (lpenvelopefollower_t *)LPMemoryPool.alloc(1, sizeof(lpenvelopefollower_t));
    env->value = 0.f;
    env->last = 0.f;
    env->phase = 0.f;
    env->interval = interval;

    return env; 
}

lpfloat_t envelopefollower_process(lpenvelopefollower_t * env, lpfloat_t input) {
    env->phase += 1;
    env->last = (lpfloat_t)fmax(env->last, (lpfloat_t)fabs(input));
    if(env->phase >= env->interval) {
        env->phase -= env->interval;        
        env->value = env->last;
    }

    return env->value;
}

void envelopefollower_destroy(lpenvelopefollower_t * env) {
    LPMemoryPool.free(env);
}

/**
 * Peak follower
 */
lppeakfollower_t * peakfollower_create(lpfloat_t interval) {
    lppeakfollower_t * peak;

    peak = (lppeakfollower_t *)LPMemoryPool.alloc(1, sizeof(lppeakfollower_t));
    peak->value = 0.f;
    peak->last = 0.f;
    peak->phase = 0.f;
    peak->interval = interval;
    peak->change = 0;

    return peak; 
}

lpfloat_t peakfollower_process(lppeakfollower_t * peak, lpfloat_t input) {
    peak->phase += 1;
    peak->last = lpfmax(peak->last, input);
    if(peak->phase >= peak->interval) {
        peak->change = 1;
        peak->phase -= peak->interval;        
        peak->value = peak->last;
    } else {
        peak->change = 0;
    }

    return peak->value;
}

void peakfollower_destroy(lppeakfollower_t * peak) {
    LPMemoryPool.free(peak);
}


/**
 * Zero crossing stream follower
 */
lpcrossingfollower_t * crossingfollower_create() {
    lpcrossingfollower_t * c;

    c = (lpcrossingfollower_t *)LPMemoryPool.alloc(1, sizeof(lpcrossingfollower_t));
    c->value = 0;
    c->lastsign = 0;
    c->in_transition = 0;
    c->ws_transition = 0;
    c->num_crossings = 3; 
    c->crossing_count = 0;

    return c; 
}

lpfloat_t crossingfollower_process(lpcrossingfollower_t * c, lpfloat_t input) {
    int current;
    current = signbit(input); 
    c->in_transition = 0;
    c->ws_transition = 0;
    if((c->lastsign && !current) || (!c->lastsign && current)) {
        c->crossing_count += 1;
        c->value = current;
        c->in_transition = 1;

        if(c->crossing_count >= c->num_crossings) {
            c->ws_transition = 1;
            c->crossing_count = 0;
        }
    }
    c->lastsign = current;
    return c->value;
}

void crossingfollower_destroy(lpcrossingfollower_t * crossing) {
    LPMemoryPool.free(crossing);
}



const lpmir_pitch_factory_t LPPitchTracker = { yin_create, yin_process, yin_destroy };
const lpmir_onset_factory_t LPOnsetDetector = { coyote_create, coyote_process, coyote_destroy };
const lpmir_envelopefollower_factory_t LPEnvelopeFollower = { envelopefollower_create, envelopefollower_process, envelopefollower_destroy };
const lpmir_peakfollower_factory_t LPPeakFollower = { peakfollower_create, peakfollower_process, peakfollower_destroy };
const lpmir_crossingfollower_factory_t LPCrossingFollower = { crossingfollower_create, crossingfollower_process, crossingfollower_destroy };

