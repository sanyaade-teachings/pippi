#include "pippi.h"
#include "ugens.sine.h"
#include "ugens.utils.h"

#define BS 4096
#define SR 48000
#define CHANNELS 2

int main() {
    lpfloat_t minfreq, maxfreq, amp, sample, freq, a, b;
    size_t i, c, length;
    lpbuffer_t * freq_lfo;
    lpbuffer_t * out;
    ugen_t * sineA;
    ugen_t * sineB;
    ugen_t * mult;

    minfreq = 80.0;
    maxfreq = 800.0;
    amp = 0.2;

    length = 10 * SR;

    /* Make an LFO table to use as a frequency curve for the osc */
    freq_lfo = LPWindow.create(WIN_SINE, BS);

    /* Scale it from a range of -1 to 1 to a range of minfreq to maxfreq */
    LPBuffer.scale(freq_lfo, 0, 1, minfreq, maxfreq);

    out = LPBuffer.create(length, CHANNELS, SR);
    sineA = create_sine_ugen();
    sineB = create_sine_ugen();
    mult = create_mult_ugen();

    for(i=0; i < length; i++) {
        freq = LPInterpolation.linear_pos(freq_lfo, (double)i/length);
        sineA->set_param(sineA, USINEIN_FREQ, &freq);
        sineA->process(sineA);
        sineB->process(sineB);

        a = sineA->get_output(sineA, USINEOUT_MAIN);
        b = sineB->get_output(sineB, USINEOUT_MAIN);

        mult->set_param(mult, UMULTIN_A, &a);
        mult->set_param(mult, UMULTIN_B, &b);
        mult->process(mult);

        sample = mult->get_output(mult, 0) * amp;
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = sample;
        }
    }

    LPSoundFile.write("renders/ugen-mult-out.wav", out);

    sineA->destroy(sineA);
    sineB->destroy(sineB);
    mult->destroy(mult);
    LPBuffer.destroy(out);
    LPBuffer.destroy(freq_lfo);

    return 0;
}
