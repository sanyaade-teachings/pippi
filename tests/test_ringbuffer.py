from unittest import TestCase
import random
import numpy as np
from pippi.soundbuffer import RingBuffer, SoundBuffer
from pippi import dsp

class TestRingBuffer(TestCase):
    def test_create_empty_ring_buffer(self):
        samplerate = 44100
        length = samplerate * 40
        channels = 2

        buf = RingBuffer(length, channels, samplerate)
        self.assertTrue(len(buf) == length)

    def test_create_ringbuffer_from_soundfile(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        buf = RingBuffer(len(sound), sound.channels, sound.samplerate, sound.frames)
        self.assertTrue(len(buf) == len(sound))

    def test_stream_blocks_into_ringbuffer_from_soundfile(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        long_buf_length = int(len(sound) * random.triangular(2, 4))
        short_buf_length = int(len(sound) * random.triangular(0.2, 0.9))
        long_buf = RingBuffer(long_buf_length, sound.channels, sound.samplerate)
        short_buf = RingBuffer(short_buf_length, sound.channels, sound.samplerate)

        pos = 0
        block_size = 64
        elapsed = 0

        while elapsed < len(sound):
            start = pos * block_size
            long_buf.write(sound.frames[start:start+block_size])
            short_buf.write(sound.frames[start:start+block_size])
            elapsed += block_size
            pos += 1

    def test_read_streamed_blocks_from_ringbuffer(self):
        sound = SoundBuffer(filename='tests/sounds/guitar1s.wav')
        long_buf_length = int(len(sound) * random.triangular(2, 4))
        short_buf_length = int(len(sound) * random.triangular(0.2, 0.9))
        long_buf = RingBuffer(long_buf_length, sound.channels, sound.samplerate)
        short_buf = RingBuffer(short_buf_length, sound.channels, sound.samplerate)

        pos = 0
        block_size = 64
        elapsed = 0

        while elapsed < len(sound):
            start = pos * block_size
            long_buf.write(sound.frames[start:start+block_size])
            short_buf.write(sound.frames[start:start+block_size])
            elapsed += block_size
            pos += 1

        full_buffer = long_buf.read(len(sound))
        part_buffer = short_buf.read(len(sound))

        self.assertEqual(len(full_buffer), len(sound))
        self.assertTrue(np.array_equal(full_buffer, sound.frames))

        self.assertEqual(len(part_buffer), len(short_buf))
        self.assertTrue(np.array_equal(part_buffer, sound.frames[-len(short_buf):]))


