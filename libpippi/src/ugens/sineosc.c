#include "pippi.h"
#include "ugens.sine.h"

#define BS 4096
#define SR 48000
#define CHANNELS 2

int main() {
    lpfloat_t minfreq, maxfreq, amp, sample;
    size_t i, c, length;
    lpbuffer_t * freq_lfo;
    lpbuffer_t * out;
    ugen_t * u;

    minfreq = 80.0;
    maxfreq = 800.0;
    amp = 0.2;

    length = 10 * SR;

    /* Make an LFO table to use as a frequency curve for the osc */
    freq_lfo = LPWindow.create(WIN_SINE, BS);

    /* Scale it from a range of -1 to 1 to a range of minfreq to maxfreq */
    LPBuffer.scale(freq_lfo, 0, 1, minfreq, maxfreq);

    out = LPBuffer.create(length, CHANNELS, SR);
    u = create_sine_ugen();

    for(i=0; i < length; i++) {
        //osc->freq = LPInterpolation.linear_pos(freq_lfo, (double)i/length);
        u->process(u);
        sample = u->get_output(u, 0) * amp;
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = sample;
        }
    }

    LPSoundFile.write("renders/ugen-sineosc-out.wav", out);

    u->destroy(u);
    LPBuffer.destroy(out);
    LPBuffer.destroy(freq_lfo);

    return 0;
}
