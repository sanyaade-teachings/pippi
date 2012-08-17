import cmd
import os
import sys
import math
import dsp
import alsaaudio
import time
from multiprocessing import Process, Queue

class Pippi(cmd.Cmd):
    """ Pippi Console 
    """

    prompt = 'pippi: '
    intro = 'Pippi Console'

    playing = {}
    cc = {} 
    vcount = 0

    def __init__(self):
        cmd.Cmd.__init__(self)

    def out(self, play, args, q):
        regen = False

        for arg in args:
            if arg == 'regen':
                regen = True
       
        snd = play(args)
        loop = True
        vol = 1.0
        tvol = 1.0

        out = alsaaudio.PCM(alsaaudio.PCM_PLAYBACK, alsaaudio.PCM_NORMAL)
        out.setchannels(2)
        out.setrate(44100)
        out.setformat(alsaaudio.PCM_FORMAT_S16_LE)
        out.setperiodsize(10)
        snd = dsp.split(snd, 50)

        while loop == True:
            if regen == True:
                next = play(args)

            for s in snd:
                if q.empty() != True:
                    params = q.get()
                    for i in params:
                        if i == 'loop':
                            loop = params[i]

                        if i == 'vol':
                            if params[i] != vol:
                                tvol = params[i]

                if tvol != vol:
                    if tvol < vol:
                        vol -= 0.0001
                    elif tvol > vol:
                        vol += 0.0001

                    s = dsp.amp(s, vol)

                out.write(s)
            
            if regen == True:
                snd = dsp.split(next, 50)

    def play(self, cmd):
        orcs = os.listdir('orc/')
        for orc in orcs:
            if cmd[0] == orc[0:2]:
                t = cmd[0]
                cmd.pop(0)
                orc = 'orc.' + orc.split('.')[0]
                p = __import__(orc, globals(), locals(), ['play'])

                for c in self.cc:
                    if c == t or c == 'g':
                        cmd = cmd + self.cc[c]

                q = Queue()
                process = Process(target=self.out, args=(p.play, cmd, q))
                process.start()

                self.vcount += 1
                self.playing[self.vcount] = {'p': process, 't': t, 'q': q, 'vol': 1.0, 'cmd': cmd}

                return True

        print 'not found'

    def do_c(self, cmd):
        cmd = cmd.split(' ')
        t = cmd[0]
        cmd.pop(0)
        self.cc[t] = cmd

    def do_ss(self, cmd):
        for s in self.playing:
            self.playing[s]['p'].terminate()
    
        self.playing = {}

    def do_s(self, cmd):
        cmds = cmd.split(' ')

        try:
            for cmd in cmds:
                self.playing[int(cmd)]['p'].terminate()
                self.playing.pop(int(cmd))
        except KeyError:
            print 'nope'

    def do_vv(self, cmd):
        cmds = cmd.split(' ')

        for p in self.playing:
            if self.playing[p]['t'] == cmds[0] or cmds[0] == 'a':
                vol = float(cmds[1]) / 100.0
                self.playing[p]['q'].put({'vol': vol})
                self.playing[p]['vol'] = vol


    def do_v(self, cmd):
        cmds = cmd.split(' ')
        try:
            vol = float(cmds[1]) / 100.0
            i = int(cmds[0])
            self.playing[i]['q'].put({'vol': vol})
            self.playing[i]['vol'] = vol
        except KeyError:
            print 'nope, nope'
        except IndexError:
            print 'nope'

    def do_i(self, cmd=[]):
        try:
            for p in self.playing:
                print p, self.playing[p]['t'], self.playing[p]['vol'], self.playing[p]['p'].exitcode, self.playing[p]['cmd']
        except KeyError:
            print 'nope'

    def do_p(self, cmd):
        if cmd == '1a':
            cmd = [
                    'sp v:50 w:60 m:150 regen o:1 n:g wf:sine',
                    'sp v:60 w:6 m:150 regen o:1 n:g wf:tri',
                    ]

        elif cmd == '1b':
            cmd = [
                    'sp v:40 w:1200 m:500 regen o:1 n:g wf:tri',
                    'sp v:30 w:2200 m:700 regen o:1 n:g wf:sine',
                    ]

        elif cmd == '1c':
            cmd = [
                    'sh v:50 regen o:2 e:phasor i:r pad:5000 t:12 n:g',
                    'sh v:50 regen o:2 e:phasor i:r pad:5000 t:12 n:g',
                    'sp w:5 m:15 v:15 o:2 wf:vary regen n:g',
                    ]

        elif cmd == '2a':
            cmd = [
                    'sp v:30 w:1200 m:400 regen o:1 n:d wf:tri tr:just',
                    'sp v:40 w:3200 m:300 regen o:1 n:d wf:sine tr:just',
                    'sp v:30 w:3200 m:300 regen o:1 n:d wf:sine tr:just',
                    'sp v:20 w:2200 m:500 regen o:1 n:d wf:tri tr:just',
                    ]

        elif cmd == '2b':
            cmd = [
                    'sh v:5 regen o:3 e:vary i:r pad:1000 t:12 n:d tr:just',
                    'sh v:5 regen o:3 e:vary i:r pad:1000 t:15 n:d tr:just',
                    'sh v:30 regen o:1 s:1.8 e:phasor i:r pad:2000 t:12 n:d tr:just',
                    'sh v:20 regen o:2 s:1.8 e:sine i:c pad:2000 t:12 n:d tr:just',
                    ]

        elif cmd == '2c':
            cmd = [
                    'sh v:55 o:3 s:1 i:r regen e:phasor pad:2000 t:15 n:d tr:just',
                    'sh v:60 o:2 s:1.6.5 i:r regen e:phasor pad:2000 t:15 n:d tr:just',
                    'sh v:35 o:3 s:1.5 i:r regen e:sine pad:1000 t:10 n:d tr:just gg:300 gp:500 ge',
                    'sh v:20 o:3 i:r regen e:sine pad:1000 t:10 n:d tr:just gg:200 gp:800 ge',
                    ]

        elif cmd == '3a':
            cmd = [
                    'dr wf:sine o:1 n:d h:1 t:30 v:50',
                    'dr wf:sine o:2 n:d h:1 t:25 v:30',
                    'dr wf:sine o:1 n:a h:1 t:20 v:30',
                    ]

        elif cmd == '3b':
            cmd = [
                    'dr wf:tri o:1 n:d h:1.2.3.4 t:35 v:50',
                    'dr wf:vary o:1 n:d h:1.2.3.4.5.6 t:25 v:30',
                    'dr wf:sine o:2 n:a h:1.2 t:27 v:40',
                    'dr wf:sine o:2 n:d h:1.2.3 v:50',
                    ]

        elif cmd == '3c':
            cmd = [
                    'dr wf:vary o:1 n:d h:1.2.3.4.5.6 t:30 v:50',
                    'dr wf:tri o:1 n:d h:1.2.3.4.5.6 t:20 v:50',
                    'dr wf:sine o:2 n:a h:1.2.3 t:24 v:50',
                    ]

        elif cmd == '4a':
            cmd = [
                    'sh v:50 o:3 s:1.5 i:r regen t:10 n:d tr:just gg:300 gp:50 ge p',
                    'sh v:40 o:3 i:g regen t:12 n:d tr:just gg:500 gp:50 ge p',
                    'sh v:50 o:2 i:r regen t:12 n:d tr:just gg:300 p',
                    ]

        for c in cmd:
            if len(c) > 2:
                self.default(c)

    def default(self, cmd):
        self.play(cmd.split(' '))

    def do_EOF(self, line):
        return True

    def postloop(self):
        pass

if __name__ == '__main__':
        # Create console
        console = Pippi()

        # Start looping command prompt
        console.cmdloop()
