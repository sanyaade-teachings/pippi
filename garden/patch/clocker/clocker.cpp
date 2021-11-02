#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

static void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    for(size_t i = 0; i < size; i++)
        for(size_t c = 0; c < 4; c++) {
            out[c][i] = 0.f;
        }
    }
}

int main(void) {
    patch.Init();

    patch.StartAdc();
    patch.StartAudio(AudioCallback);

    while(1) {
        patch.DisplayControls(false);
    }
}

