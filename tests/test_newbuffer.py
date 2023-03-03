from os import path
import random
import shutil
import tempfile
from unittest import TestCase

from pippi.buffers import SoundBuffer
from pippi import dsp

class TestNewBuffer(TestCase):
    def setUp(self):
        self.soundfiles = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.soundfiles)

    def test_create_empty_buffer(self):
        sound = SoundBuffer()

        self.assertTrue(len(sound) == 0)
        self.assertTrue(not sound)

        sound = SoundBuffer(length=1)
        self.assertEqual(len(sound), 44100)
        self.assertTrue(sound)

    def test_mul_scalar(self):
        snd = SoundBuffer([1,2,3])
        result = SoundBuffer([2,4,6])
        self.assertEqual(len(snd), 3)
        self.assertEqual(snd * 2, result)
        self.assertEqual(snd, SoundBuffer([1,2,3]))

    def test_mul_soundbuffers(self):
        snd = SoundBuffer([1,2,3])
        snd2 = SoundBuffer([1,3,5])
        result = SoundBuffer([1,6,15])
        self.assertEqual(snd * snd2, result)
        self.assertEqual(snd, SoundBuffer([1,2,3]))

    def test_rmul_soundbuffers(self):
        snd = SoundBuffer([1,2,3])
        snd2 = SoundBuffer([1,3,5])
        result = SoundBuffer([1,6,15])

        self.assertEqual(snd2 * snd, result)
        self.assertEqual(snd, SoundBuffer([1,2,3]))

    def test_imul_scalar(self):
        snd = SoundBuffer([1,2,3])
        result = SoundBuffer([2,4,6])
        snd *= 2
        self.assertEqual(snd, result)

    def test_imul_soundbuffer(self):
        snd = SoundBuffer([1,2,3])
        mul = SoundBuffer([2,2,2])
        result = SoundBuffer([2,4,6])
        snd *= mul
        self.assertEqual(snd, result)

    def test_add_soundbuffers(self):
        snd = SoundBuffer([1,2,3])
        self.assertEqual(len(snd), 3)
        self.assertEqual(snd + 2, SoundBuffer([3,4,5]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        self.assertEqual(snd + SoundBuffer([1,3,5]), SoundBuffer([1,2,3,1,3,5]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        self.assertEqual(SoundBuffer([1,3,5]) + snd, SoundBuffer([1,3,5,1,2,3]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        snd += 2
        self.assertEqual(snd, SoundBuffer([3,4,5]))

    def test_sub_soundbuffers(self):
        snd = SoundBuffer([1,2,3])
        self.assertEqual(len(snd), 3)

        self.assertEqual(snd - 1.5, SoundBuffer([-0.5, 0.5, 1.5]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        self.assertEqual(snd - 2, SoundBuffer([-1,0,1]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        self.assertEqual(snd - SoundBuffer([1,3,5]), SoundBuffer([0,-1,-2]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        self.assertEqual(SoundBuffer([1,3,5]) - snd, SoundBuffer([0,1,2]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        snd -= 2
        self.assertEqual(snd, SoundBuffer([-1,0,1]))

    def test_div_soundbuffers(self):
        snd = SoundBuffer([1,2,3])
        self.assertEqual(len(snd), 3)

        self.assertEqual(snd / 1.5, SoundBuffer([1/1.5, 2/1.5, 3/1.5]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        self.assertEqual(snd / 2, SoundBuffer([0.5, 1, 1.5]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        self.assertEqual(snd / 0, SoundBuffer([0,0,0]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        self.assertEqual(snd / SoundBuffer([1,3,5]), SoundBuffer([1/1, 2/3, 3/5]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        self.assertEqual(SoundBuffer([1,3,5]) / snd, SoundBuffer([1/1, 3/2, 5/3]))
        self.assertEqual(snd, SoundBuffer([1,2,3]))

        snd /= 2
        self.assertEqual(snd, SoundBuffer([1/2, 2/2, 3/2]))


    def test_slice_frame(self):
        # A SoundBuffer should return a single frame 
        #    when sliced into one-dimensionally like:
        #
        #        frame = sound[frame_index]
        #
        #    A frame is a tuple of floats, one value 
        #    for each channel of sound.
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        indices = (0, -1, len(sound) // 2, -(len(sound) // 2))

        for frame_index in indices:
            frame = sound[frame_index]
            self.assertTrue(isinstance(frame, tuple))
            self.assertEqual(len(frame), sound.channels)
            self.assertTrue(isinstance(frame[0], float))

    def test_slice_sample(self):
        # Slicing into the second dimension of a SoundBuffer
        #    will return a single sample at the given channel index.
        #
        #        sample = sound[frame_index][channel_index]
        #
        #    Note: A sample is a float, usually between -1.0 and 1.0 
        #    but pippi will only clip overflow when you ask it to, or 
        #    when writing a SoundBuffer back to a file. 
        #    So, numbers can exceed that range during processing and 
        #    be normalized or clipped as desired later on.

        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        indices = (0, -1, len(sound) // 2, -(len(sound) // 2))

        for frame_index in indices:
            for channel_index in range(sound.channels):
                sample = sound[frame_index][channel_index]
                self.assertTrue(isinstance(sample, float))

    def test_mix_operator(self):
        snd1 = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        snd2 = SoundBuffer(filename='tests/sounds/LittleTikes-A1.wav')

        snd1 &= snd2

        self.assertEqual(len(snd1), len(snd2))

    def test_mix_mismatched_channels(self):
        snd1 = SoundBuffer(filename='tests/sounds/guitar1s-mono.wav')
        snd2 = SoundBuffer(filename='tests/sounds/LittleTikes-A1.wav')
        channels = max(snd1.channels, snd2.channels)

        out = snd1 & snd2
        self.assertEqual(out.channels, channels)

        out = snd2 & snd1
        self.assertEqual(out.channels, channels)

        snd2 &= snd1
        self.assertEqual(snd2.channels, channels)
        self.assertEqual(snd1.channels, 1) # Right side should have no side effects

        snd1 &= snd2
        self.assertEqual(snd1.channels, channels)

    def test_copy_soundbuffer(self):
        snd = SoundBuffer(filename='tests/sounds/LittleTikes-A1.wav')
        snd2 = snd.copy()
        self.assertEqual(len(snd), len(snd2))

    def test_create_stereo_buffer_from_soundfile(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        self.assertEqual(len(sound), 44100)
        self.assertTrue(sound.samplerate == 44100)

        sound = dsp.read('tests/sounds/guitar1s.wav')
        self.assertEqual(len(sound), 44100)
        self.assertTrue(sound.samplerate == 44100)

    def test_graph_soundfile(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        sound.graph('tests/renders/newsoundbuffer_graph.png', width=1280, height=800)

    def test_max_soundfile(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        self.assertTrue(sound.max > 0)

    def test_min_soundfile(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        self.assertTrue(sound.min < 0)

    def test_mag_soundfile(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        self.assertTrue(sound.mag > 0)

    def test_create_mono_buffer_from_soundfile(self):
        sound = SoundBuffer(filename='tests/sounds/linux.wav')
        self.assertTrue(sound.samplerate == 44100)
        self.assertTrue(sound.channels == 1)
        self.assertEqual(len(sound), 228554)

        sound = dsp.read('tests/sounds/linux.wav')
        self.assertTrue(sound.samplerate == 44100)
        self.assertTrue(sound.channels == 1)
        self.assertEqual(len(sound), 228554)

    def test_create_mono_buffer_from_wavetable(self):
        wt = dsp.wt('sine', wtsize=4096)
        self.assertTrue(len(wt) == 4096)

        snd = SoundBuffer(wt)
        self.assertTrue(len(snd) == 4096)
        self.assertTrue(snd[100][0] != 0)

        snd = SoundBuffer(wt)
        self.assertTrue(len(snd) == 4096)
        self.assertTrue(snd[100][0] != 0)


    def test_dub_scalar_into_mono_buffer(self):
        snd = SoundBuffer(length=1, samplerate=20, channels=1)
        snd.dub(1, framepos=10)
        self.assertTrue(snd[10] == (1.,))

    def test_dub_scalar_into_stereo_buffer(self):
        snd = SoundBuffer(length=1, samplerate=20, channels=2)
        snd.dub(1, framepos=10)
        self.assertTrue(snd[10] == (1.,1.))

    def test_dub_scalar_into_quad_buffer(self):
        snd = SoundBuffer(length=1, samplerate=20, channels=4)
        snd.fdub(1, 10)
        self.assertTrue(snd[10] == (1.,1.,1.,1.))

    def test_dub_into_buffer_at_framepos(self):
        snd = SoundBuffer(filename='tests/sounds/guitar10s.wav')
        src = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        snd.fdub(src, 1000)
        snd.write('tests/renders/test_newbuffer_dub.wav')
        snd.graph('tests/renders/newbuffer-dub-at-offset.png')

    def test_dub_into_empty_buffer(self):
        src = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        snd = SoundBuffer(length=src.dur)
        snd.dub(src, 0)

    def test_dub_into_mono_buffer(self):
        snd = SoundBuffer([0,0,0])
        snd.dub(SoundBuffer([1,3,5]))
        self.assertEqual(snd, SoundBuffer([1,3,5]))

    def test_clear_buffer(self):
        snd = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        snd.clear()
        self.assertTrue(snd[100][0] == 0)

    def test_cut_buffer(self):
        snd = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        bit = snd.cut(0, 0.5)
        self.assertTrue(bit.dur == 0.5)
        self.assertTrue(snd[100][0] == bit[100][0])

    def test_fcut_buffer(self):
        snd = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        bit = snd.fcut(10, 500)
        self.assertTrue(len(bit) == 500)
        self.assertTrue(snd[100][0] == bit[90][0])

    def test_rcut_buffer(self):
        snd = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        bit = snd.rcut(0.1)
        self.assertTrue(bit.dur == 0.1 or bit.dur == snd.dur)

    def test_window(self):
        snd = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        snd.env('sine').write('tests/renders/newbuffer-env-sine.wav')
        snd.graph('tests/renders/newbuffer-env-sine.png')

    def test_remix_soundbuffer(self):
        snd = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        self.assertEqual(snd.remix(1).channels, 1)
        self.assertEqual(snd.remix(4).channels, 4)

    def test_plot_soundbuffer(self):
        snd = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        #snd.plot()

    def test_save_buffer_to_soundfile(self):
        filename = path.join(self.soundfiles, 'test_save_newbuffer_to_soundfile.{}')
        sound = SoundBuffer(length=1)

        sound.write(filename.format('wav'))
        self.assertTrue(path.isfile(filename.format('wav')))

        sound.write(filename.format('flac'))
        self.assertTrue(path.isfile(filename.format('flac')))

        sound.write(filename.format('ogg'))
        self.assertTrue(path.isfile(filename.format('ogg')))

    def test_convolve_soundbuffer(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        impulse = SoundBuffer(filename='tests/sounds/LittleTikes-A1.wav')
        out = sound.convolve(impulse)
        out.write('tests/renders/newbuffer_convolve_guitar_littletikes.wav')

    def test_repeat_soundbuffer(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        out = sound.repeat(5)
        out.write('tests/renders/newbuffer_repeat.wav')

    def test_reverse_soundbuffer(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        sound.reverse()
        sound.write('tests/renders/newbuffer_reverse.wav')

    def test_reversed_soundbuffer(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        out = sound.reversed()
        out.write('tests/renders/newbuffer_reversed.wav')

    def test_overfill_soundbuffer(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        length = 2.45
        out = sound.fill(length)
        self.assertEqual(out.dur, length)
        out.write('tests/renders/newbuffer_overfill.wav')

    def test_underfill_soundbuffer(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        length = sound.dur / 3
        out = sound.fill(length)
        self.assertEqual(out.dur, length)
        out.write('tests/renders/newbuffer_underfill.wav')

    def test_clip_soundbuffer(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        sound = sound.clip(-0.1, 0.1)

        self.assertEqual(len(sound), 44100)
        self.assertTrue(sound.samplerate == 44100)
        self.assertTrue(sound.max <= 0.1)

    def test_split_into_blocks(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav').cut(0, 0.11)
        blocksize = 2048
        numblocks = len(sound) // blocksize
        slop = len(sound) % blocksize
        count = 1
        for block in sound.blocks(blocksize):
            if count == numblocks:
                self.assertTrue(len(block), slop)
            else:
                self.assertTrue(len(block), blocksize)
            count += 1

    def test_split_buffer(self):
        def _split(length):
            sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

            total = 0
            for grain in sound.grains(length):
                total += len(grain)

            if length <= 0:
                self.assertEqual(total, 0)
                return

            # Check that the remainder grain is not 0
            self.assertNotEqual(len(grain), 0)

            # Check that all the grains add up
            self.assertEqual(total, len(sound))

        lengths = [-1, 0, 0.01, 0.121212, 0.3, 0.5599, 0.75, 0.999, 1, 2]
        for length in lengths:
            _split(length)

    def test_random_split_buffer(self):
        for _ in range(10):
            sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

            total = 0
            for grain in sound.grains(0.001, sound.dur):
                total += len(grain)

            # Check that the remainder grain is not 0
            self.assertNotEqual(len(grain), 0)

            # Check that all the grains add up
            self.assertEqual(total, len(sound))

    def test_pad_sound_with_silence(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        # Pad before
        original_length = len(sound)
        silence_length = random.triangular(0.001, 1)
        sound = sound.pad(silence_length)
        sound.write('tests/renders/newbuffer_pad_before.wav')

        self.assertEqual(len(sound), int((sound.samplerate * silence_length) + original_length))
        self.assertEqual(sound[0], (0,0))

        # Pad after
        original_length = len(sound)
        silence_length = random.triangular(0.001, 1)
        sound = sound.pad(after=silence_length)
        sound.write('tests/renders/newbuffer_pad_after.wav')

        self.assertEqual(len(sound), int((sound.samplerate * silence_length) + original_length))
        self.assertEqual(sound[-1], (0,0))


"""
    def test_stack_soundbuffer(self):
        snd1 = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        snd2 = SoundBuffer(filename='tests/sounds/LittleTikes-A1.wav')
        channels = snd1.channels + snd2.channels
        length = max(len(snd1), len(snd2))
        out = dsp.stack([snd1, snd2])
        self.assertTrue(channels == out.channels)
        self.assertTrue(length == len(out))
        self.assertTrue(snd1.samplerate == out.samplerate)
        out.write('tests/renders/soundbuffer_stack.wav')

    def test_speed(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        speed = random.random()
        out = sound.speed(speed)
        out.write('tests/renders/soundbuffer_speed.wav')
        self.assertEqual(len(out), int(len(sound) * (1/speed)))

    def test_vpeed(self):
        sound = SoundBuffer(filename='tests/sounds/linux.wav')
        speed = dsp.win('hann', 0.5, 2)
        out = sound.vspeed(speed)
        out.write('tests/renders/soundbuffer_vspeed_0.5_2.wav')

        speed = dsp.win('hann', 0.15, 0.5)
        out = sound.vspeed(speed)
        out.write('tests/renders/soundbuffer_vspeed_0.15_0.5.wav')

        speed = dsp.win('hann', 5, 50)
        out = sound.vspeed(speed)
        out.write('tests/renders/soundbuffer_vspeed_5_50.wav')


    def test_transpose(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')

        speed = random.triangular(1, 10)
        out = sound.transpose(speed)
        self.assertEqual(len(out), len(sound))

        speed = random.triangular(0, 1)
        out = sound.transpose(speed)
        self.assertEqual(len(out), len(sound))

        speed = random.triangular(10, 100)
        out = sound.transpose(speed)
        self.assertEqual(len(out), len(sound))


    def test_pan(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        for pan_method in ('linear', 'constant', 'gogins'):
            # Hard pan smoke test
            pan_left = sound.pan(0, method=pan_method)
            self.assertEqual(pan_left[random.randint(0, len(pan_left))][0], 0)

            pan_right = sound.pan(1, method=pan_method)
            self.assertEqual(pan_right[random.randint(0, len(pan_right))][1], 0)

    def test_trim_silence(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav').env('hannout')
        firstval = abs(sum(sound[0]))
        lastval = abs(sum(sound[-1]))
        sound = sound.pad(start=1, end=1)

        self.assertEqual(sound.dur, 3)

        sound.write('tests/renders/trim_silence_before.wav')

        for threshold in (0, 0.01, 0.5):
            trimstart = sound.trim(start=True, end=False, threshold=threshold)
            trimstart.write('tests/renders/trim_silence_start%s.wav' % threshold)

            trimend = sound.trim(start=False, end=True, threshold=threshold)
            trimend.write('tests/renders/trim_silence_end%s.wav' % threshold)

    def test_taper(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        sound = sound.taper(0)
        sound = sound.taper(0.001)
        sound = sound.taper(0.01)
        sound = sound.taper(0.1)
        sound = sound.taper(1)
        sound = sound.taper(10)

        self.assertEqual(len(sound), 44100)
        self.assertTrue(sound.samplerate == 44100)
"""
