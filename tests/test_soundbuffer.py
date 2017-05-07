from os import path
import random
import shutil
import tempfile
from unittest import TestCase

from pippi.soundbuffer import SoundBuffer

class TestSoundBuffer(TestCase):
    def setUp(self):
        self.soundfiles = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.soundfiles)

    def test_create_empty_buffer(self):
        sound = SoundBuffer()
        self.assertTrue(len(sound) == 0)
        self.assertTrue(not sound)

        sound = SoundBuffer(length=44100)
        self.assertEqual(len(sound), 44100)
        self.assertTrue(sound)

    def test_create_buffer_from_soundfile(self):
        sound = SoundBuffer('tests/sounds/guitar1s.wav')

        self.assertTrue(len(sound) > 0)
        self.assertTrue(sound.samplerate == 44100)

    def test_create_and_resize_buffer_from_soundfile(self):
        length = random.randint(1, 44100)
        sound = SoundBuffer('tests/sounds/guitar1s.wav', length)

        self.assertEqual(len(sound), length)

    def test_save_buffer_to_soundfile(self):
        filename = path.join(self.soundfiles, 'test_save_buffer_to_soundfile.{}')
        sound = SoundBuffer(length=44100)

        sound.write(filename.format('wav'))
        self.assertTrue(path.isfile(filename.format('wav')))

        sound.write(filename.format('flac'))
        self.assertTrue(path.isfile(filename.format('flac')))

        sound.write(filename.format('ogg'))
        self.assertTrue(path.isfile(filename.format('ogg')))

    def test_split_buffer(self):
        sound = SoundBuffer('tests/sounds/guitar1s.wav')

        length = random.randint(1, len(sound)) 
        lengths = []
        for grain in sound.grains(length):
            lengths += [ len(grain) ]

        # The final grain isn't padded with silence, 
        # so it should only be the grain length if 
        # it can be divided equally into the total 
        # length.
        for grain_length in lengths[:-1]:
            self.assertEqual(grain_length, length)

        # Check that the remainder grain is the correct length
        self.assertEqual(lengths[-1], len(sound) - sum(lengths[:-1]))

        # Check that all the grains add up
        self.assertEqual(sum(lengths), len(sound))

    def test_random_split_buffer(self):
        sound = SoundBuffer('tests/sounds/guitar1s.wav')

        lengths = []
        for grain in sound.grains(1, len(sound)):
            lengths += [ len(grain) ]

        # Check that the remainder grain is not 0
        self.assertNotEqual(lengths[-1], 0)

        # Check that all the grains add up
        self.assertEqual(sum(lengths), len(sound))

