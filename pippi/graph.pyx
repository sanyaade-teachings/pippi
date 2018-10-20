# cython: language_level=3, profile=True
import random
from PIL import Image, ImageDraw
from pippi import interpolation
from pippi import dsp
from pippi.soundbuffer cimport SoundBuffer
import sys
       
cpdef void waveform(SoundBuffer sound, str filename=None, int width=400, int height=300, int stroke=3, int upsample_mult=5, int upsample_x=20, bint show_axis=True):
    cdef int i = 0
    cdef int channel = 0
    cdef int pos = 0
    cdef double point = 0
    cdef tuple color
    cdef double[:] points
    cdef list mapped_points

    width *= upsample_mult
    height *= upsample_mult

    if filename is None:
        filename = 'waveform.png'

    x = range(width*upsample_x)

    img = Image.new('RGBA', (width, height), (255, 255, 255, 255))
    draw = ImageDraw.Draw(img)

    for channel in range(sound.channels):
        color = tuple([random.randint(0, 200) for _ in range(3)] + [200])
        points = interpolation.linear([ sound[i][channel] for i in range(len(sound)) ], width * upsample_x)

        mapped_points = []
        for pos, point in zip(x, points):
            y = <int>(((point + 1) / 2) * (height - (stroke * 2)) + stroke)
            pos /= upsample_x
            mapped_points += [ (pos, y) ]
            draw.ellipse((pos-stroke, y-stroke, pos+stroke, y+stroke), fill=color)
        draw.line(mapped_points, fill=color, width=stroke)

    if show_axis:
        draw.line((0, height/2, width, height/2), fill=(0,0,0,255), width=stroke//4)

    img.thumbnail((width//upsample_mult, height//upsample_mult))
    img.save(filename)

if __name__ == '__main__':
    snd_filename = sys.argv[1]
    out_filename = sys.argv[2]
    snd = dsp.read(snd_filename).remix(1)
    waveform(snd, out_filename, width=2050, height=1024)
