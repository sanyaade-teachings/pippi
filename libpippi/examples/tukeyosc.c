#include "pippi.h"

#define BS 4096
#define SR 48000
#define CHANNELS 2

int main() {
    lpfloat_t amp, sample;
    size_t i, c, length;
    lpbuffer_t * freq_lfo;
    lpbuffer_t * shape_lfo;
    lpbuffer_t * out;
    lptukeyosc_t * osc;

    amp = 0.2;

    length = 10 * SR;

    /* Make an LFO table to use as a frequency curve for the osc */
    freq_lfo = LPWindow.create(WIN_SINE, BS);
    /* Scale it from a range of -1 to 1 to a range of minfreq to maxfreq */
    LPBuffer.scale(freq_lfo, 0, 1, 80.f, 200.f);

    shape_lfo = LPWindow.create(WIN_SINE, BS);
    LPBuffer.scale(shape_lfo, 0, 1, 0.f, 1.f);

    out = LPBuffer.create(length, CHANNELS, SR);
    osc = LPTukeyOsc.create();
    osc->samplerate = SR;

    for(i=0; i < length; i++) {
        osc->freq = LPInterpolation.linear_pos(freq_lfo, (double)i/length);
        osc->shape = LPInterpolation.linear_pos(shape_lfo, (double)i/length);
        sample = LPTukeyOsc.process(osc) * amp;
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = sample;
        }
    }

    LPSoundFile.write("renders/tukeyosc-out.wav", out);

    LPTukeyOsc.destroy(osc);
    LPBuffer.destroy(out);
    LPBuffer.destroy(freq_lfo);

    return 0;
}
