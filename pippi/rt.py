import os
from pippi import dsp
from pippi import param
import multiprocessing as mp
import alsaaudio

def grid(tick, bpm):
    os.nice(-19)
    
    bpm = dsp.mstf(dsp.bpm2ms(bpm))

    while True:
        tick.set()
        tick.clear()
        dsp.delay(bpm)

def render(play, buffers, params, voice_id):
    buffer = getattr(buffers, voice_id)
    buffer = dsp.split(play(params), 500)
    setattr(buffers, voice_id, buffer)

def dsp_loop(out, buffer, params, voice_params, voice_id):
    target_volume = params.get('target_volume', 1.0)
    volume        = params.get('post_volume', 1.0)

    for chunk in buffer:
        if target_volume != volume:
            if target_volume > volume:
                volume += 0.01
            elif volume > target_volume:
                volume -= 0.01

            chunk = dsp.amp(chunk, volume)

        out.write(chunk)

        if volume < 0.002:
            params['loop']   = False
            params['post_volume'] = volume
            setattr(voice_params, voice_id, params)
            break

def out(generator, buffers, voice_params, voice_id, tick):
    """ Master playback process spawned by play()
        Manages render and playback processes  
        """

    # Give this process a high priority to help prevent unwanted audio glitching
    os.nice(-19)

    # Fetch voice params from namespace
    params = getattr(voice_params, voice_id)

    # Spawn a render process which will write generator output
    # into the buffer for this voice
    r = mp.Process(target=render, args=(generator.play, buffers, params, voice_id))
    r.start()
    r.join()

    # Fetch the buffer that was just filled
    buffer = getattr(buffers, voice_id)

    # Open a connection to an ALSA PCM device
    device = params.get('device', 'default')

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
    cooking             = False # Flag set to true if a render subprocess has been spawned
    volume              = 1.0
    next                = False

    while params['loop'] == True:
            
        regenerate    = params.get('regenerate', False)
        quantize      = params.get('quantize', False)
        target_volume = params.get('target_volume', 1.0)

        if regenerate is True and cooking is False:
            cooking = True
            generator = reload(generator)
            next = mp.Process(target=render, args=(generator.play, buffers, params, voice_id))
            next.start()

            if params.get('once', False) == True:
                next.join()

        if hasattr(next, 'is_alive') and next.is_alive() is False:
            cooking = False
            buffer = getattr(buffers, voice_id)

        if quantize is not False:
            tick.wait()

        dsp_loop(out, buffer, params, voice_params, voice_id)
        params = getattr(voice_params, voice_id)

    # Cleanup 
    delattr(voice_params, voice_id)
    delattr(buffers, voice_id)


