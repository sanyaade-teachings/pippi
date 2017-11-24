from os import path
import random
import shutil
import tempfile
from unittest import TestCase

from pippi.soundbuffer import SoundBuffer
from pippi import dsp

class TestDub(TestCase):
    def setUp(self):
        self.soundfiles = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.soundfiles)

    def test_dub_into_empty_sound(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        out = SoundBuffer(channels=sound.channels, samplerate=sound.samplerate)
        self.assertEqual(len(out), 0)
        self.assertEqual(len(sound), 44100)

        position = random.triangular(0.1, 1)
        out.dub(sound, pos=position)

        self.assertEqual(len(out), int((position * sound.samplerate) + len(sound)))
        self.assertEqual(sound.channels, out.channels)

    def test_dub_overflow(self):
        sound = dsp.read('tests/sounds/guitar1s.wav')
        out = dsp.buffer()

        numdubs = 3
        maxpos = 4
        for _ in range(numdubs):
            pos = random.triangular(0, maxpos)
            out.dub(sound, pos)

        self.assertTrue(len(out) <= (maxpos * out.samplerate) + len(sound))
