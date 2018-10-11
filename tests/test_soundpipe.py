from unittest import TestCase
from pippi import soundpipe, dsp, fx, wavetables

class TestSoundpipe(TestCase):
    def test_compressor(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        ratio = 4
        threshold = -10
        attack = 0.2
        release = 0.2

        snd = soundpipe.compressor(snd, ratio, threshold, attack, release)
        snd.write('tests/renders/soundpipe_compressor.wav')

    def test_paulstretch(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        windowsize = 1
        stretch = 10

        snd = soundpipe.paulstretch(snd, windowsize, stretch)
        snd.write('tests/renders/soundpipe_paulstretch.wav')

    def test_mincer(self):
        snd = dsp.read('examples/sounds/linus.wav')
        length = 20
        time = wavetables.randline(10, lowvalue=0.5, highvalue=2.5)
        #time = dsp.PHASOR
        amp = 1
        pitch = wavetables.randline(10)
        #pitch = 1.0

        snd = soundpipe.mincer(snd, snd.dur, time, amp, pitch)
        snd.write('tests/renders/soundpipe_mincer.wav')


    def test_saturator_nodc(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        drive = 10
        dcoffset = 0
        dcblock = False

        snd = soundpipe.saturator(snd, drive, dcoffset, dcblock)
        snd = fx.norm(snd, 1)
        snd.write('tests/renders/soundpipe_saturator_nodc.wav')

    def test_saturator_dc(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        drive = 10
        dcoffset = 0.1
        dcblock = True

        snd = soundpipe.saturator(snd, drive, dcoffset, dcblock)
        snd.write('tests/renders/soundpipe_saturator_dc.wav')

    def test_butlp(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        freq = 100

        snd = soundpipe.butlp(snd, freq)
        snd.write('tests/renders/soundpipe_butlp.wav')


    def test_buthp(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        freq = 1000

        snd = soundpipe.buthp(snd, freq)
        snd.write('tests/renders/soundpipe_buthp.wav')

    def test_butbp(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        freq = 500

        snd = soundpipe.butbp(snd, freq)
        snd.write('tests/renders/soundpipe_butbp.wav')

    def test_butbr(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        freq = 500

        snd = soundpipe.butbr(snd, freq)
        snd.write('tests/renders/soundpipe_butbr.wav')

