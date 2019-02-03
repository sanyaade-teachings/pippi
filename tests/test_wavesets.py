import random
from unittest import TestCase

from pippi.oscs import Pulsar2d
from pippi.wavesets import Waveset
from pippi.soundbuffer import SoundBuffer
from pippi import dsp

class TestWavesets(TestCase):
    def test_pulsar_from_waveset(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav').cut(0, 1)
        waveset = Waveset(sound, limit=20, modulo=3)
        osc = Pulsar2d(
                waveset,
                windows=[dsp.SINE], 
                wt_mod=dsp.wt(dsp.SAW, 0, 1), 
                win_mod=0.0, 
                pulsewidth=1.0, 
                freq=200.0, 
                amp=0.2
            )

        length = 30
        out = osc.play(length)
        out.write('tests/renders/waveset_pulsar2d_wavetables.wav')
        self.assertEqual(len(out), int(length * out.samplerate))



