from unittest import TestCase
from pippi import dsp, fft, fx

class TestFFT(TestCase):
    def test_fft_convolve_real(self):
        g = dsp.read('tests/sounds/guitar1s.wav')
        l = dsp.read('tests/sounds/LittleTikes-B1.wav')

        out = g.convolve(l)
        out.write('tests/renders/fft_conv.wav')


    def test_fft_transform(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        mod = dsp.win('sine')

        # Transform
        real, imag = fft.transform(snd)

        # Do stuff
        imag = real * mod
        real = imag * mod

        # Inverse Transform
        out = fft.itransform(real, imag)
        out = fx.norm(out, 1)
        out.write('tests/renders/fft_transform.wav')



