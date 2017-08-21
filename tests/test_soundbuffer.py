from os import path
import random
import shutil
import tempfile
from unittest import TestCase

from pippi.soundbuffer import SoundBuffer
from pippi import dsp

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
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        self.assertEqual(len(sound), 44100)
        self.assertTrue(sound.samplerate == 44100)

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
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

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
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        lengths = []
        for grain in sound.grains(1, len(sound)):
            lengths += [ len(grain) ]

        # Check that the remainder grain is not 0
        self.assertNotEqual(lengths[-1], 0)

        # Check that all the grains add up
        self.assertEqual(sum(lengths), len(sound))

    def test_window(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        print('FROM PY', len(sound))
        for window_type in ('sine', 'saw', 'tri', 'hamm', 'hann', 'bar', 'kai', 'black'):
            sound = sound.env(window_type)
            self.assertEqual(sound[0], (0,0))

    def test_speed(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        speed = random.random()
        out = sound.speed(speed)
        self.assertEqual(len(out), int(len(sound) * (1/speed)))

    def test_pan(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        for pan_method in ('linear', 'constant', 'linear'):
            # Hard pan smoke test
            pan_left = sound.pan(0, method=pan_method)
            self.assertEqual(pan_left[random.randint(0, len(pan_left))][0], 0)

            pan_right = sound.pan(1, method=pan_method)
            self.assertEqual(pan_right[random.randint(0, len(pan_right))][1], 0)

    def test_slice_frame(self):
        """ A SoundBuffer should return a single frame 
            when sliced into one-dimensionally like:

                frame = sound[frame_index]

            A frame is a tuple of floats, one value 
            for each channel of sound.
        """
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        indices = (0, -1, len(sound) // 2, -(len(sound) // 2))

        for frame_index in indices:
            frame = sound[frame_index]
            self.assertTrue(isinstance(frame, tuple))
            self.assertEqual(len(frame), sound.channels)
            self.assertTrue(isinstance(frame[0], float))

    def test_slice_sample(self):
        """ Slicing into the second dimension of a SoundBuffer
            will return a single sample at the given channel index.

                sample = sound[frame_index][channel_index]

            Note: A sample is a float, usually between -1.0 and 1.0 
            but pippi will only clip overflow when you ask it to, or 
            when writing a SoundBuffer back to a file. 
            So, numbers can exceed that range during processing and 
            be normalized or clipped as desired later on.
        """

        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        indices = (0, -1, len(sound) // 2, -(len(sound) // 2))

        for frame_index in indices:
            for channel_index in range(sound.channels):
                sample = sound[frame_index][channel_index]
                self.assertTrue(isinstance(sample, float))

    def test_pad_sound_with_silence(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        # Pad start
        original_length = len(sound)
        silence_length = random.randint(100, 44100)
        sound = sound.pad(silence_length)

        self.assertEqual(len(sound), silence_length + original_length)
        self.assertEqual(sound[0], (0,0))

        # Pad end
        original_length = len(sound)
        silence_length = random.randint(100, 44100)
        sound = sound.pad(end=silence_length)

        self.assertEqual(len(sound), silence_length + original_length)
        self.assertEqual(sound[-1], (0,0))

    def test_dub_into_empty_sound(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        original_length = len(sound)

        out = SoundBuffer(channels=sound.channels, samplerate=sound.samplerate)
        self.assertEqual(len(out), 0)
        self.assertEqual(len(sound), 44100)

        position = random.randint(100, 1000)

        out.dub(sound, pos=position)

        self.assertEqual(len(out), original_length + position)
        self.assertEqual(sound.channels, out.channels)

