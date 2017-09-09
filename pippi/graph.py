import random
from PIL import Image, ImageDraw
from . import interpolation

def waveform(sound, filename=None, width=400, height=300, stroke=3, upsample_mult=5, upsample_x=20, show_axis=True):
    width *= upsample_mult
    height *= upsample_mult

    if filename is None:
        filename = 'waveform.png'

    x = range(width*upsample_x)

    img = Image.new('RGBA', (width, height), (255, 255, 255, 0))
    draw = ImageDraw.Draw(img)

    for channel in range(sound.channels):
        color = tuple([random.randint(0, 200) for _ in range(3)] + [200])
        points = [ sound[i][channel] for i in range(len(sound)) ]
        points = interpolation.linear(points, width*upsample_x)

        mapped_points = []
        for pos, point in zip(x, points):
            y = int(((point + 1) / 2) * (height - (stroke * 2)) + stroke)
            pos /= upsample_x
            mapped_points += [ (pos, y) ]
            #draw.ellipse((pos-stroke, y-stroke, pos+stroke, y+stroke), fill=color)
        draw.line(mapped_points, fill=color, width=stroke)

    if show_axis:
        draw.line((0, height/2, width, height/2), fill=(0,0,0,255), width=stroke//4)

    img.thumbnail((width//upsample_mult, height//upsample_mult))
    img.save(filename)

