import random
from unittest import TestCase

from pippi.oscs import Pulsar2d
from pippi.wavesets import Waveset
from pippi.soundbuffer import SoundBuffer
from pippi import dsp

class TestWavesets(TestCase):
    def test_pulsar_from_waveset(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        waveset = Waveset(sound, limit=20, modulo=1)
        waveset.normalize()
        osc = Pulsar2d(
                waveset,
                windows=[dsp.SINE], 
                wt_mod=dsp.wt(dsp.SAW, 0, 1), 
                win_mod=0.0, 
                pulsewidth=1.0, 
                freq=80.0, 
                amp=0.8
            )

        length = 30
        out = osc.play(length)
        out.write('tests/renders/waveset_pulsar2d_wavetables-80hz.wav')
        self.assertEqual(len(out), int(length * out.samplerate))


    def test_render(self):
        sound = SoundBuffer(filename='tests/sounds/guitar10s.wav')
        waveset = Waveset(sound)
        out = waveset.render(channels=1)
        out.write('tests/renders/waveset_render.wav')
