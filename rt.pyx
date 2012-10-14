import os
import dsp
import multiprocessing as mp
import alsaaudio

def grid(tick, bpm):
    os.nice(-19)
    
    bpm = dsp.mstf(dsp.bpm2ms(bpm))
    #bpm = bpm / 4 

    while True:
        tick.set()
        tick.clear()
        dsp.delay(bpm)


def render(play, args, vid, voices, label='snd'):
    voice = getattr(voices, str(vid))
    voice[label] = dsp.split(play(args), 500)
    setattr(voices, str(vid), voice)

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

def out(play, gen, voices, vid, tick):
    """
        Worker playback process spawned by play()
        Manages render threads for voices and render buffers for playback
    """

    os.nice(-19)

    voice = getattr(voices, str(vid))

    # Render sound
    r = mp.Process(target=render, args=(play, voice['cmd'], vid, voices))
    r.start()
    r.join()

    # Open a connection to an ALSA PCM device
    # Split buffered sound into packets and push to playback
    if gen == 'sh':
        device = 'T6_pair1'
    elif gen == 'sp':
        device = 'T6_pair3'
    elif gen == 'dr':
        device = 'T6_pair2'
    elif gen == 'cl':
        device = 'T6_pair3'
    else:
        device = 'default'

    #device = 'default'

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
            
        for arg in voice['cmd']:
            if arg == 're':
                regen = True

            if arg == 'one':
                onegen = True
                voice['cmd'].remove('one')

            if arg[:2] == 'qu':
                q = True

        tvol = voice['tvol']

        if (regen == True or onegen == True) and cooking == False:
            cooking = True
            next = mp.Process(target=render, args=(play, voice['cmd'], vid, voices, 'next'))
            next.start()

            if onegen == True:
                onegen = False

        if next is not False and next.is_alive() == False:
            cooking = False
            snd = voice['next']
            setattr(voices, str(vid), voice)
        else:
            snd = voice['snd']

        if q is not False:
            tick.wait()

        dsp_loop(out, snd, vol, tvol, voice, voices, vid)

        voice = getattr(voices, str(vid))

    # Cleanup 
    delattr(voices, str(vid))


