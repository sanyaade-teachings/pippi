#include "pippi.h"
#include "ugens.tape.h"


int main() {
    lpfloat_t sample;
    size_t i;
    int c;
    lpbuffer_t * out;
    lpbuffer_t * buf;
    ugen_t * u;


    buf = LPSoundFile.read("../tests/sounds/living.wav");

    out = LPBuffer.create(10 * buf->samplerate, buf->channels, buf->samplerate);
    u = create_tape_ugen();
    u->set_param(u, UTAPEIN_BUF, (void *)buf);

    for(i=0; i < out->length; i++) {
        u->process(u);
        sample = u->get_output(u, 0);
        for(c=0; c < buf->channels; c++) {
            out->data[i * buf->channels + c] = sample;
        }
    }

    LPSoundFile.write("renders/ugen-tapeosc-out.wav", out);

    u->destroy(u);
    LPBuffer.destroy(out);
    LPBuffer.destroy(buf);

    return 0;
}
