from unittest import TestCase
from pippi import soundpipe, dsp, fx, wavetables
import numpy as np

class TestSoundpipe(TestCase):
    def test_compressor(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        ratio = 4
        threshold = -10
        attack = 0.2
        release = 0.2

        snd.frames = soundpipe.compressor(snd.frames, ratio, threshold, attack, release)
        snd.write('tests/renders/soundpipe_compressor.wav')

    def test_paulstretch(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        windowsize = 1
        stretch = 10

        snd.frames = soundpipe.paulstretch(snd.frames, windowsize, stretch, snd.samplerate)
        snd.write('tests/renders/soundpipe_paulstretch.wav')

    def test_mincer(self):
        snd = dsp.read('tests/sounds/linux.wav')
        length = 20
        amp = 1
        time = dsp.randline(10, 0.5, 2)
        pitch = dsp.randline(10)

        snd.frames = soundpipe.mincer(snd.frames, snd.dur, time.data, amp, pitch.data, samplerate=snd.samplerate)
        snd.write('tests/renders/soundpipe_mincer.wav')

    def test_saturator_nodc(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        drive = 10
        dcoffset = 0
        dcblock = False

        snd.frames = soundpipe.saturator(snd.frames, drive, dcoffset, dcblock)
        snd = fx.norm(snd, 1)
        snd.write('tests/renders/soundpipe_saturator_nodc.wav')

    def test_saturator_dc(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        drive = 10
        dcoffset = 0.1
        dcblock = True

        snd.frames = soundpipe.saturator(snd.frames, drive, dcoffset, dcblock)
        snd.write('tests/renders/soundpipe_saturator_dc.wav')

    def test_butlp(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        freq = np.array([100], dtype='d')

        snd.frames = soundpipe.butlp(snd.frames, freq)
        snd.write('tests/renders/soundpipe_butlp.wav')

    def test_buthp(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        freq = np.array([1000], dtype='d')

        snd.frames = soundpipe.buthp(snd.frames, freq)
        snd.write('tests/renders/soundpipe_buthp.wav')

    def test_butbp(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        freq = np.array([500], dtype='d')

        snd.frames = soundpipe.butbp(snd.frames, freq)
        snd.write('tests/renders/soundpipe_butbp.wav')

    def test_butbr(self):
        snd = dsp.read('tests/sounds/guitar1s.wav')
        freq = np.array([500], dtype='d')

        snd.frames = soundpipe.butbr(snd.frames, freq)
        snd.write('tests/renders/soundpipe_butbr.wav')

