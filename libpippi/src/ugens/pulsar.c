#include "pippi.h"
#include "ugens.pulsar.h"

#define SR 48000
#define CHANNELS 2


int main() {
    lpfloat_t sample;
    size_t i;
    int c, num_win, num_wts;
    lpbuffer_t * out;
    lpbuffer_t * wts;
    lpbuffer_t * win;
    ugen_t * u;

    num_wts = 3;
    num_win = 1;

    size_t wt_offsets[num_wts] = {};
    size_t wt_lengths[num_wts] = {};
    size_t win_offsets[num_win] = {};
    size_t win_lengths[num_win] = {};

    size_t * wt_offsetsp = wt_offsets;
    size_t * wt_lengthsp = wt_lengths;
    size_t * win_offsetsp = win_offsets;
    size_t * win_lengthsp = win_lengths;

    wts = LPWavetable.create_stack(num_wts, 
        wt_offsets, wt_lengths,
        WT_SINE, 4096,
        WT_RND, 4096,
        WT_RND, 4096
    );

    win = LPWindow.create_stack(num_win, 
            win_offsets, win_lengths,
            WIN_HANN, 4096
    );

    out = LPBuffer.create(10 * SR, CHANNELS, SR);
    u = create_pulsar_ugen();

    u->set_param(u, UPULSARIN_WTTABLE, (void *)(&wts->data));
    u->set_param(u, UPULSARIN_WTTABLELENGTH, (void *)(&wts->length));
    u->set_param(u, UPULSARIN_NUMWTS, (void *)(&num_wts));
    u->set_param(u, UPULSARIN_WTOFFSETS, (void *)(&wt_offsetsp));
    u->set_param(u, UPULSARIN_WTLENGTHS, (void *)(&wt_lengthsp));

    u->set_param(u, UPULSARIN_WINTABLE, (void *)(&win->data));
    u->set_param(u, UPULSARIN_WINTABLELENGTH, (void *)(&win->length));
    u->set_param(u, UPULSARIN_NUMWINS, (void *)(&num_win));
    u->set_param(u, UPULSARIN_WINOFFSETS, (void *)(&win_offsetsp));
    u->set_param(u, UPULSARIN_WINLENGTHS, (void *)(&win_lengthsp));

    for(i=0; i < out->length; i++) {
        u->process(u);
        sample = u->get_output(u, 0);
        for(c=0; c < CHANNELS; c++) {
            out->data[i * CHANNELS + c] = sample;
        }
    }

    LPSoundFile.write("renders/ugen-pulsarosc-out.wav", out);

    u->destroy(u);
    LPBuffer.destroy(out);
    LPBuffer.destroy(wts);
    LPBuffer.destroy(win);

    return 0;
}
