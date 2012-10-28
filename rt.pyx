import os
import dsp
import multiprocessing as mp
import alsaaudio
import param

def grid(tick, bpm):
    os.nice(-19)
    
    bpm = dsp.mstf(dsp.bpm2ms(bpm))

    while True:
        tick.set()
        tick.clear()
        dsp.delay(bpm)

def render(play, voices, voice_id, buffer='snd'):
    voice = getattr(voices, str(voice_id))

    voice[buffer] = dsp.split(play(voice['params']), 500)
    setattr(voices, str(voice_id), voice)


def dsp_loop(out, snd, vol, tvol, voice, voices, vid):
    for s in snd:
        if tvol != vol:
            if tvol > vol:
                vol += 0.01
            elif vol > tvol:
                vol -= 0.01

            s = dsp.amp(s, vol)

        out.write(s)

        if vol < 0.002:
            voice['loop'] = False
            setattr(voices, str(vid), voice)
            break
    return (vol, tvol)

def out(play, voices, vid, tick):
    """
        Worker playback process spawned by play()
        Manages render threads for voices and render buffers for playback
    """

    os.nice(-19)

    voice = getattr(voices, str(vid))
    gen = voice['params']['generator']

    # Render sound
    r = mp.Process(target=render, args=(play, voices, vid))
    r.start()
    r.join()

    # Open a connection to an ALSA PCM device
    # Split buffered sound into packets and push to playback
    if gen == 'shine':
        device = 'T6_pair1'
    elif gen == 'sparkle':
        device = 'T6_pair3'
    elif gen == 'drone':
        device = 'T6_pair2'
    elif gen == 'click':
        device = 'T6_pair3'
    else:
        device = 'default'

    try:
        out = alsaaudio.PCM(alsaaudio.PCM_PLAYBACK, alsaaudio.PCM_NORMAL, device)
    except:
        print 'Could not open an ALSA connection.'
        return False

    out.setchannels(2)
    out.setrate(44100)
    out.setformat(alsaaudio.PCM_FORMAT_S16_LE)
    out.setperiodsize(10)

    # On start of playback, check to see if we should be regenerating 
    # the sound. If so, spawn a new render thread.
    # If there is a fresher render available, use that instead.
    regen = False
    onegen = False
    cooking = False
    next = False
    q = False
    vol = 1.0
    tvol = 1.0

    while voice['loop'] == True:
            
        for arg in voice['params']:
            if arg == 'regenerate':
                regen = True

            if arg == 'one':
                onegen = True
                voice['cmd'].remove('one')

            if arg == 'quantize':
                q = True

        tvol = voice['tvol']

        if (regen == True or onegen == True) and cooking == False:
            cooking = True
            next = mp.Process(target=render, args=(play, voices, vid, 'snd'))
            next.start()

            if onegen == True:
                onegen = False

        if next is not False and next.is_alive() == False:
            cooking = False
            snd = voice['snd']
            setattr(voices, str(vid), voice)
        else:
            snd = voice['snd']

        if q is not False:
            tick.wait()

        vol, tvol = dsp_loop(out, snd, vol, tvol, voice, voices, vid)

        voice = getattr(voices, str(vid))

    # Cleanup 
    delattr(voices, str(vid))


