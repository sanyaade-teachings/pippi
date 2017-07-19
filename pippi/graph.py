import random
from PIL import Image, ImageDraw
from . import interpolation

UPSAMPLE_MULT = 10

def waveform(sound, filename=None, width=400, height=200, stroke=None):
    width *= UPSAMPLE_MULT
    height *= UPSAMPLE_MULT

    if stroke is None:
        stroke = UPSAMPLE_MULT

    if filename is None:
        filename = 'waveform.png'

    x = range(width)

    img = Image.new('RGBA', (width, height), (255, 255, 255, 0))
    draw = ImageDraw.Draw(img)

    for channel in range(sound.channels):
        color = tuple([random.randint(0, 255) for _ in range(3)] + [255])
        points = [ sound[i][channel] for i in range(len(sound)) ]
        points = interpolation.linear(points, width)

        for pos, point in zip(x, points):
            y = int(((point + 1) / 2) * (height - (stroke * 2)) + stroke)
            draw.ellipse((pos-stroke, y-stroke, pos+stroke, y+stroke), fill=color)

    img.thumbnail((width//UPSAMPLE_MULT, height//UPSAMPLE_MULT))
    img.save(filename)
