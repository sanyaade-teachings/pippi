import alsaaudio

def capture(length=44100, device='default', numchans=2):
    input = alsaaudio.PCM(alsaaudio.PCM_CAPTURE, 0, device)
    input.setchannels(numchans)
    input.setrate(44100)
    input.setformat(alsaaudio.PCM_FORMAT_S16_LE)
    input.setperiodsize(100)

    out = ''
    count = 0
    while count < length:
        rec_frames, rec_data = input.read()
        count += rec_frames
        out += rec_data

    return out

