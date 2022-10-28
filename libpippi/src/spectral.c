#include "spectral.h"

/* FIXME: handle multichannel audio */
lpbuffer_t * convolve_spectral(lpbuffer_t * src, lpbuffer_t * impulse) {
    size_t length, i;
    int c;
    lpfloat_t mag;
    lpbuffer_t * out;
    lpbuffer_t * outa;
    lpbuffer_t * outb;
    lpbuffer_t * srca;
    lpbuffer_t * srcb;
    lpbuffer_t * impulsea;
    lpbuffer_t * impulseb;

    assert(src->channels == 1 || src->channels == 2);
    assert(impulse->channels == 1 || impulse->channels == 2);
    assert(impulse->channels == src->channels);

    length = src->length + impulse->length + 1;
    out = LPBuffer.create(length, src->channels, src->samplerate);

    mag = LPBuffer.mag(src);

    if(src->channels == 2) {
        outa = LPBuffer.create(length, src->channels, src->samplerate);
        outb = LPBuffer.create(length, src->channels, src->samplerate);

        srca = LPBuffer.create(src->length, 1, src->samplerate);
        srcb = LPBuffer.create(src->length, 1, src->samplerate);
        LPBuffer.split2(src, srca, srcb);

        impulsea = LPBuffer.create(impulse->length, 1, src->samplerate);
        impulseb = LPBuffer.create(impulse->length, 1, src->samplerate);
        LPBuffer.split2(impulse, impulsea, impulseb);

        Fft_convolveReal(srca->data, impulsea->data, outa->data, length);
        Fft_convolveReal(srcb->data, impulseb->data, outb->data, length);

        for(i=0; i < length; i++) {
            for(c=0; c < out->channels; c++) {
                out->data[i * out->channels] = outa->data[i];
                out->data[i * out->channels + 1] = outb->data[i];
            }
        }

        LPBuffer.destroy(outa);
        LPBuffer.destroy(outb);
        LPBuffer.destroy(srca);
        LPBuffer.destroy(srcb);
        LPBuffer.destroy(impulsea);
        LPBuffer.destroy(impulseb);
    } else {
        Fft_convolveReal(src->data, impulse->data, out->data, length);
    }

    LPFX.norm(out, mag);

    return out;
}

const lpspectral_factory_t LPSpectral = { convolve_spectral };
