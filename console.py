import cmd
import os
import sys
import math
import dsp
import alsaaudio
import time
import multiprocessing as mp

class Pippi(cmd.Cmd):
    """ Pippi Console 
    """

    prompt = 'pippi: '
    intro = 'Pippi Console'

    cc = {} 
    vid = 0

    def __init__(self):
        cmd.Cmd.__init__(self)

        self.server = mp.Manager()
        self.voices = self.server.Namespace()

        self.vid = 0

        self.qu = {
                1: mp.Event(),
                4: mp.Event(),
                8: mp.Event(),
                16: mp.Event(),
            }

        self.grid = mp.Process(target=self.timer, args=(self.qu,))
        self.grid.start()

    def timer(self, qu):
        os.nice(-19)
        bar = 0

        while True:
            bpm = (dsp.bpm2ms(75.0) / 1000.0) * 0.25
            for beat in range(1, 17):
                dsp.delay(bpm)

                # signal 4 beats
                if beat == 1:
                    bar += 1

                if bar == 1 and beat == 1:
                    qu[1].set()

                if bar == 4 and beat == 1:
                    bar = 0

                # signal 1 beat
                if beat == 1:
                    qu[4].set()

                # signal 0.5 beats
                if beat % 2 == 0:
                    qu[8].set()

                # signal 0.25 beats
                qu[16].set()

                for q in qu:
                    qu[q].clear()


    def render(self, play, args, vid, voices, label='snd'):
        voice = getattr(voices, str(vid))
        voice[label] = dsp.split(play(args), 500)
        setattr(voices, str(vid), voice)

    def out(self, play, args, gen, voices, vid, qu):
        """
            Worker playback process spawned by play()
            Manages render threads for voices and render buffers for playback
        """

        # Render sound
        render = mp.Process(target=self.render, args=(play, args, vid, voices))
        render.start()
        render.join()

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
        cooking = False
        next = False
        q = False
        vol = 1.0
        tvol = 1.0

        for arg in args:
            if arg == 're':
                regen = True

            if arg[:2] == 'qu':
                q = int(arg[3:].strip())

        voice = getattr(voices, str(vid))

        while voice['loop'] == True:
            tvol = voice['tvol']

            if regen == True and cooking == False:
                cooking = True
                next = mp.Process(target=self.render, args=(play, args, vid, voices, 'next'))
                next.start()

            if next is not False and next.is_alive() == False:
                cooking = False
                snd = voice['next']
                setattr(voices, str(vid), voice)
            else:
                snd = voice['snd']

            if q is not False:
                qu[q].wait()

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

            voice = getattr(voices, str(vid))

        # Cleanup 
        delattr(voices, str(vid))


    def play(self, cmd):
        orcs = os.listdir('orc/')
        for orc in orcs:
            if cmd[0] == orc[0:2]:
                gen = cmd[0]
                cmd.pop(0)
                orc = 'orc.' + orc.split('.')[0]
                pl = __import__(orc, globals(), locals(), ['play'])

                for c in self.cc:
                    if c == t or c == 'g':
                        cmd = cmd + self.cc[c]

                # Create a shared list for the voice
                self.vid += 1
                print self.vid, gen, cmd
                voice_id = self.vid

                voice = {'snd': '', 'next': '', 'loop': True, 'regen': False, 'tvol': 1.0, 'cmd': cmd, 'gen': gen}
                setattr(self.voices, str(voice_id), voice)

                process = mp.Process(target=self.out, args=(pl.play, cmd, gen, self.voices, voice_id, self.qu))
                process.start()

                return True

        print 'not found'

    def do_c(self, cmd):
        cmd = cmd.split(' ')
        t = cmd[0]
        cmd.pop(0)
        self.cc[t] = cmd

    def do_ss(self, cmd):
        for vid in range(1, self.vid + 1):
            if hasattr(self.voices, str(vid)):
                voice = getattr(self.voices, str(vid))
                voice['loop'] = False
                setattr(self.voices, str(vid), voice)

    def do_s(self, cmd):
        cmds = cmd.split(' ')

        for cmd in cmds:
            vid = cmd.strip() 
            voice = getattr(self.voices, vid)
            voice['loop'] = False
            setattr(self.voices, vid, voice)


    def do_vv(self, cmd):
        cmds = cmd.split(' ')

        for vid in range(1, self.vid + 1):
            self.setvol(vid, cmds[1], cmds[0])

    def do_v(self, cmd):
        cmds = cmd.split(' ')
        self.setvol(cmds[0], cmds[1])

    def setvol(self, vid, vol, gen='a'):
        if hasattr(self.voices, str(vid)):
            voice = getattr(self.voices, str(vid))
            if gen == voice['gen'] or gen == 'a':
                voice['tvol'] = float(vol) / 100.0
                setattr(self.voices, str(vid), voice)
        else:
            print 'nope'

    def do_i(self, cmd=[]):
        for vid in range(1, self.vid + 1):
            if hasattr(self.voices, str(vid)):
                voice = getattr(self.voices, str(vid))
                print vid, voice['gen'], voice['cmd'], 'v:', voice['tvol'], 'l:', voice['loop'], 're:', voice['regen']

    def do_p(self, cmd):
        if cmd == 's1aa':
            cmd = [
                    'dr o:2 n:bb t:40 h:1 wf:sine2pi',
                    'dr o:2 n:eb t:36 h:2 wf:sine2pi',
                    'dr o:2 n:eb t:35 h:1 wf:sine2pi a v:6',
                    'dr o:2 n:eb t:25 h:2 wf:sine2pi a v:6',
                    'sp w:80 m:70 v:40 wf:sine2pi o:2 n:ab re',
                    'sp w:40 m:120 v:50 wf:sine2pi o:2 n:ab re',
                    ]

        elif cmd == 's1ab':
            cmd = [
                    'dr o:1 n:eb t:30 h:2.4 wf:tri',
                    'dr o:2 n:bb t:32 h:1.2.4 wf:sine2pi',
                    ]

        elif cmd == 's1ac':
            cmd = [
                    'dr o:4 n:eb t:27 h:8 h:1 wf:sine2pi bend re v:1',
                    'dr o:4 n:eb t:20 h:8 h:1 wf:sine2pi bend re v:1',
                    ]

        elif cmd == 's1ba':
            cmd = [
                    #'dr o:4 n:eb t:27 h:8 h:1 wf:sine2pi bend re v:1',
                    ]

        elif cmd == 's1bb':
            cmd = [
                    #'dr o:4 n:eb t:27 h:8 h:1 wf:sine2pi bend re v:1',
                    ]

        elif cmd == 's1ca':
            cmd = [
                    #'dr o:4 n:eb t:27 h:8 h:1 wf:sine2pi bend re v:1',
                    ]

        elif cmd == 's1cb':
            cmd = [
                    #'dr o:4 n:eb t:27 h:8 h:1 wf:sine2pi bend re v:1',
                    ]


        elif cmd == 's2aa':
            cmd = [
                    'sh o:2 n:db s:1 qu:8 t:0.06 r:1',
                    'sh o:2 n:db s:5 qu:8 t:0.08 r:1',
                    ]

        elif cmd == 's2ab':
            cmd = [
                    'cl w:10 d:h qu:8 b:1',
                    ]

        elif cmd == 's2ac':
            cmd = [
                    'sh o:2 n:db s:8 qu:8 t:0.1 r:1',
                    'cl d:h qu:16 b:1',
                    ]




        for c in cmd:
            if len(c) > 2:
                self.default(c)
                dsp.delay(0.2)

    def default(self, cmd):
        cmd = cmd.strip().split(',')
        for c in cmd:
            cc = c.strip().split(' ')
            try:
                getattr(self, 'do_' + cc[0])(c[3:])

            except:
                self.play(cc)


    def do_EOF(self, line):
        return True

    def postloop(self):
        pass

if __name__ == '__main__':
        # Create console
        console = Pippi()

        # Start looping command prompt
        console.cmdloop()
