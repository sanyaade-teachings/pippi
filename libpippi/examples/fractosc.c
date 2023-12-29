#include "pippi.h"

#define BS 4096
#define SR 48000
#define CHANNELS 2

int main() {
    lpfloat_t minfreq, maxfreq, amp, sample;
    size_t i, c, length;
    lpbuffer_t * freq_lfo;
    lpbuffer_t * depth_lfo;
    lpbuffer_t * out;
    lpfractosc_t * osc;

    minfreq = 420.0;
    maxfreq = 440.0;
    amp = 0.2;

    length = 10 * SR;

    /* Make LFO tables to use as a frequency and depth curves for the osc */
    freq_lfo = LPWindow.create(WIN_SINE, BS);
    depth_lfo = LPWindow.create(WIN_SINE, BS);

    /* Scale it from a range of -1 to 1 to a range of minfreq to maxfreq */
    LPBuffer.scale(freq_lfo, 0, 1, minfreq, maxfreq);
    LPBuffer.scale(depth_lfo, 0, 1, 0, 0.1);

    out = LPBuffer.create(length, CHANNELS, SR);
    osc = LPFractOsc.create();
    osc->samplerate = SR;

    for(i=0; i < length; i++) {
        osc->freq = LPInterpolation.linear_pos(freq_lfo, (double)i/length);
        osc->depth = LPInterpolation.linear_pos(depth_lfo, (double)i/length);
        sample = LPFractOsc.process(osc) * amp;
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = sample;
        }
    }

    LPSoundFile.write("renders/fractosc-out.wav", out);

    LPFractOsc.destroy(osc);
    LPBuffer.destroy(out);
    LPBuffer.destroy(freq_lfo);
    LPBuffer.destroy(depth_lfo);

    return 0;
}
