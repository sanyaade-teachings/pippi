# cython: language_level=3, profile=True
import random
from PIL import Image, ImageDraw
from pippi cimport interpolation
from pippi cimport dsp
from pippi.soundbuffer cimport SoundBuffer
from pippi.wavetables cimport Wavetable
import sys
import numpy as np

cpdef void write(object data, 
        object filename=None, 
        int width=400, 
        int height=300, 
        double offset=1, 
        double mult=0.5, 
        int stroke=50, 
        int upsample_mult=5, 
        bint show_axis=True):

    cdef int i = 0
    cdef int channels
    cdef int channel = 0
    cdef double pos = 0
    cdef double point = 0
    cdef tuple color
    cdef double[:,:] _data
    cdef double[:] points
    cdef list mapped_points
    cdef list x_axis
    cdef int x

    if isinstance(data, SoundBuffer):
        channels = data.channels
        _data = data.frames

    elif isinstance(data, Wavetable):
        channels = 1
        _data = np.transpose(np.array(data.data, dtype='d', ndmin=2))

    elif isinstance(data, list):
        channels = 1
        _data = np.transpose(np.array(data, dtype='d', ndmin=2))

    else:
        channels = data.shape[1]
        _data = data

    width *= upsample_mult
    height *= upsample_mult
    stroke *= upsample_mult

    if filename is None:
        filename = 'waveform.png'

    x_axis = list(range(width))

    img = Image.new('RGBA', (width, height), (255, 255, 255, 255))
    draw = ImageDraw.Draw(img)

    for channel in range(channels):
        color = tuple([random.randint(0, 200) for _ in range(3)] + [200])

        points = (np.array(_data[:,channel], dtype='d') + offset) * height * mult

        mapped_points = []
        for x in x_axis:
            pos = x / width
            y = interpolation._linear_pos(points, pos)
            mapped_points += [ (x, y) ]

        draw.line(mapped_points, fill=color, width=stroke, joint='curve')

    if show_axis:
        draw.line((0, height/2, width, height/2), fill=(0,0,0,255), width=stroke//4)

    img.thumbnail((width//upsample_mult, height//upsample_mult))
    img.save(filename)

# FIXME use `write` interface everywhere
cpdef void waveform(object sound, 
        object filename=None, 
        int width=400, 
        int height=300, 
        double lowvalue=-1,
        double highvalue=1,
        int stroke=3, 
        int upsample_mult=5, 
        bint show_axis=True):
    write(sound, filename, width, height, lowvalue, highvalue, stroke, upsample_mult, show_axis)
 
if __name__ == '__main__':
    snd_filename = sys.argv[1]
    out_filename = sys.argv[2]
    snd = dsp.read(snd_filename).remix(1)
    waveform(snd, out_filename, width=2050, height=1024)
