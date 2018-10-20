from os import path
import random
import shutil
import tempfile
from unittest import TestCase

from pippi.soundbuffer import SoundBuffer
from pippi import dsp, grains

class TestCloud(TestCase):
    def setUp(self):
        self.soundfiles = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.soundfiles)

    def test_newcloud(self):
        sound = SoundBuffer(filename='examples/sounds/linus.wav')
        cloud = grains.Cloud(sound, 
                window=dsp.HANN,
                grainlength=dsp.wt(dsp.SINE) * 0.05 + 0.001,
                spread=dsp.PHASOR,
                grid=0.04,
                )
                #mask=[0,1,0,1,1])
        out = cloud.play(10)
        out.write('tests/renders/cloud_new.wav')

    def test_unmodulated_graincloud(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        cloud = grains.Cloud(sound)

        length = random.triangular(1, 4)
        framelength = int(length * sound.samplerate)

        out = cloud.play(length)
        self.assertEqual(len(out), framelength)

        out.write('tests/renders/test_unmodulated_graincloud.flac')

    def test_minspeed_graincloud(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        cloud = grains.Cloud(sound, speed=0)

        length = random.triangular(1, 4)
        framelength = int(length * sound.samplerate)

        out = cloud.play(length)
        self.assertEqual(len(out), framelength)

        out.write('tests/renders/test_minspeed_graincloud.flac')

    def test_maxspeed_graincloud(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        cloud = grains.Cloud(sound, speed=99)

        length = random.triangular(1, 4)
        framelength = int(length * sound.samplerate)

        out = cloud.play(length)
        self.assertEqual(len(out), framelength)

        out.write('tests/renders/test_maxspeed_graincloud.flac')

    def test_graincloud_with_length_lfo(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        grainlength = dsp.wt(dsp.RND) * 0.3 + 0.01
        cloud = grains.Cloud(sound, grainlength=grainlength)

        length = random.triangular(1, 4)
        framelength = int(length * sound.samplerate)

        out = cloud.play(length)
        self.assertEqual(len(out), framelength)

        out.write('tests/renders/test_graincloud_with_length_lfo.flac')

    def test_graincloud_with_speed_lfo(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        minspeed = random.triangular(0.5, 1)
        maxspeed = minspeed + random.triangular(0.5, 1)
        speed = dsp.wt(dsp.RND) * (maxspeed - minspeed) + minspeed
        cloud = grains.Cloud(sound, speed=speed)

        length = random.triangular(1, 4)
        framelength = int(length * sound.samplerate)

        out = cloud.play(length)
        self.assertEqual(len(out), framelength)

        out.write('tests/renders/test_graincloud_with_speed_lfo.flac')

    def test_graincloud_with_read_lfo(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        cloud = grains.Cloud(sound, 
                            position=dsp.RND, 
                        )

        length = random.triangular(1, 4)
        framelength = int(length * sound.samplerate)

        out = cloud.play(length)
        self.assertEqual(len(out), framelength)

        out.write('tests/renders/test_graincloud_with_read_lfo.flac')


