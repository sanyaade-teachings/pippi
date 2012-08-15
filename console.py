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

    def do_s(self, cmd):
        cmds = cmd.split(' ')

        try:
            for cmd in cmds:
                self.playing[int(cmd)]['p'].terminate()
                self.playing.pop(int(cmd))
        except KeyError:
            print 'nope'

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
