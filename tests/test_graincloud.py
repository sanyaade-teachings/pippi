from os import path
import random
import shutil
import tempfile
from unittest import TestCase

from pippi.soundbuffer import SoundBuffer
from pippi import dsp, grains, grains2, fx, shapes

class TestCloud(TestCase):
    def setUp(self):
        self.soundfiles = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.soundfiles)

    def test_libpippi_graincloud(self):
        sound = SoundBuffer(filename='tests/sounds/living.wav')
        cloud = grains2.Cloud2(sound)

        length = 30
        framelength = int(length * sound.samplerate)

        out = cloud.play(length)
        out = fx.compressor(out*4, -15, 15)
        out = fx.norm(out, 0.5)

        out.write('tests/renders/graincloud_libpippi_unmodulated.wav')

        self.assertEqual(len(out), framelength)

    def test_grainlength_modulation(self):
        snd = dsp.read('tests/sounds/living.wav')
        grainlength = shapes.win('sine', dsp.MS*10, 0.2)
        out = snd.cloud(snd.dur*2, grainlength=grainlength)
        out.write('tests/renders/graincloud_libpippi_grainlength_modulated.wav')

    def test_user_window(self):
        snd = dsp.read('tests/sounds/living.wav')
        win = dsp.win('pluckout')
        grainlength = shapes.win('sine', dsp.MS*10, 0.2)
        out = snd.cloud(snd.dur*2, window=win, grainlength=grainlength)
        out.write('tests/renders/graincloud_libpippi_user_window.wav')

    def test_phase_modulation(self):
        snd = dsp.read('tests/sounds/living.wav')
        phase = [1,0.5,1,0,0.5]
        out = snd.cloud(snd.dur*2, grainlength=0.1, phase=phase)
        out.write('tests/renders/graincloud_libpippi_phase_modulated.wav')

    def test_phase_unmodulated(self):
        snd = dsp.read('tests/sounds/living.wav')
        out = snd.cloud(snd.dur*2, grainlength=0.1)
        out.write('tests/renders/graincloud_libpippi_phase_unmodulated.wav')


    """
    def test_libpippi_pulsed_graincloud(self):
        sound = SoundBuffer(filename='tests/sounds/living.wav')
        out = sound.cloud(10, grainlength=0.06, grid=0.12)
        out.write('tests/renders/graincloud_libpippi_pulsed.wav')

    def test_libpippi_graincloud_with_length_lfo(self):
        sound = SoundBuffer(filename='tests/sounds/living.wav')
        grainlength = dsp.wt('hann', 0.01, 0.1)
        length = 3 
        framelength = int(length * sound.samplerate)

        out = sound.cloud(length, grainlength=grainlength)

        self.assertEqual(len(out), framelength)

        out.write('tests/renders/graincloud_libpippi_with_length_lfo.wav')

    def test_libpippi_graincloud_with_speed_lfo(self):
        sound = SoundBuffer(filename='tests/sounds/living.wav')
        minspeed = random.triangular(0.05, 1)
        maxspeed = minspeed + random.triangular(0.5, 10)
        speed = dsp.wt('rnd', minspeed, maxspeed)
        cloud = grains2.Cloud2(sound, grainlength=0.04, speed=speed)

        length = 3
        framelength = int(length * sound.samplerate)

        out = cloud.play(length)
        self.assertEqual(len(out), framelength)

        out.write('tests/renders/graincloud_libpippi_with_speed_lfo.wav')

    def test_libpippi_graincloud_with_extreme_speed_lfo(self):
        sound = SoundBuffer(filename='tests/sounds/living.wav')

        length = 3
        speed = dsp.wt('hann', 1, 100)
        framelength = int(length * sound.samplerate)

        out = sound.cloud(length=length, speed=speed)
        self.assertEqual(len(out), framelength)

        out.write('tests/renders/graincloud_libpippi_with_extreme_speed_lfo.wav')

    def test_libpippi_graincloud_with_read_lfo(self):
        sound = SoundBuffer(filename='tests/sounds/living.wav')
        cloud = grains2.Cloud2(sound, position=dsp.win('hann', 0, 1))

        length = 3
        framelength = int(length * sound.samplerate)

        out = cloud.play(length)
        self.assertEqual(len(out), framelength)

        out.write('tests/renders/graincloud_libpippi_with_read_lfo.wav')

    def test_libpippi_graincloud_grainsize(self):
        snd = dsp.read('tests/sounds/living.wav')
        out = snd.cloud(
                length=dsp.rand(8, 16), 
                window='hann', 
                grainlength=dsp.win('sinc', 0.2, 6), 
                grid=dsp.win('hannout', 0.04, 1),
                spread=1, 
            )

        out.write('tests/renders/graincloud_libpippi_grainsize.wav')
    """
