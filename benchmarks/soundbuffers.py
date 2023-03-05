import timeit

SNDUP = "SND = SoundBuffer(filename='tests/sounds/guitar1s.wav')"
NPYUP = "import numpy as np; NPY = np.zeros((48000, 2))"
LISUP = "L = [(0, 0)] * 48000"
WINUP = "L = [0] * 48000"

print('libpippi buffers')
print('----------------')
print('create empty buffer %0.6f' % timeit.timeit('SoundBuffer()', setup='from pippi.buffers import SoundBuffer'))
print('create 1s buffer %0.6f' % timeit.timeit('SoundBuffer(length=1)', setup='from pippi.buffers import SoundBuffer', number=10000))
print('create buffer from 48000 frame silent numpy array %0.6f' % timeit.timeit('SoundBuffer(NPY)', setup='from pippi.buffers import SoundBuffer; ' + NPYUP, number=100))
print('create buffer from 48000 element list of tuples %0.6f' % timeit.timeit('SoundBuffer(L)', setup='from pippi.buffers import SoundBuffer; ' + LISUP, number=100))
print('create buffer from 48000 element list of zeros %0.6f' % timeit.timeit('SoundBuffer(L)', setup='from pippi.buffers import SoundBuffer; ' + WINUP, number=100))
print('read 1s soundfile into buffer %0.6f' % timeit.timeit('SoundBuffer(filename="tests/sounds/guitar1s.wav")', setup='from pippi.buffers import SoundBuffer', number=100))
print('clip to -0.1 / 0.1 %0.6f' % timeit.timeit('SND.clip(-0.1, 0.1)', setup='from pippi.buffers import SoundBuffer; ' + SNDUP, number=10000))
print()

print('numpy buffers')
print('-------------')
print('create empty buffer %0.6f' % timeit.timeit('SoundBuffer()', setup='from pippi.soundbuffer import SoundBuffer'))
print('create 1s buffer %0.6f' % timeit.timeit('SoundBuffer(length=1)', setup='from pippi.soundbuffer import SoundBuffer', number=10000))
print('create buffer from 48000 frame silent numpy array %0.6f' % timeit.timeit('SoundBuffer(NPY)', setup='from pippi.soundbuffer import SoundBuffer; ' + NPYUP, number=100))
#print('create buffer from 48000 element list of tuples %0.6f' % timeit.timeit('SoundBuffer(L, channels=2)', setup='from pippi.soundbuffer import SoundBuffer; ' + LISUP, number=100))
print('create buffer from 48000 element list of zeros %0.6f' % timeit.timeit('SoundBuffer(L)', setup='from pippi.soundbuffer import SoundBuffer; ' + WINUP, number=100))
print('read 1s soundfile into buffer %0.6f' % timeit.timeit('SoundBuffer(filename="tests/sounds/guitar1s.wav")', setup='from pippi.soundbuffer import SoundBuffer', number=100))
print('clip to -0.1 / 0.1 %0.6f' % timeit.timeit('SND.clip(-0.1, 0.1)', setup='from pippi.soundbuffer import SoundBuffer; ' + SNDUP, number=10000))
