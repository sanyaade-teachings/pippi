import cmd
import os
import sys
import math
import dsp
import alsaaudio
import time
import json
import rt
import multiprocessing as mp

class Pippi(cmd.Cmd):
    """ Pippi Console 
    """

    prompt = 'pippi: '
    intro = 'Pippi Console'

    cc = {} 
    vid = 0
    bpm = 75

    def __init__(self):
        cmd.Cmd.__init__(self)

        self.config = open('config/suite.json')
        self.config = json.load(self.config)

        self.server = mp.Manager()
        self.voices = self.server.Namespace()

        self.vid = 0

        self.tick = mp.Event()

        self.grid = mp.Process(target=rt.grid, args=(self.tick, self.bpm))
        self.grid.start()


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
                print self.vid, gen, [str(c) for c in cmd]
                voice_id = self.vid

                cmd = ['bpm:' + str(self.bpm)] + cmd

                voice = {'snd': '', 'next': '', 'loop': True, 'regen': False, 'tvol': 1.0, 'cmd': cmd, 'gen': gen}
                setattr(self.voices, str(voice_id), voice)

                process = mp.Process(target=rt.out, args=(pl.play, gen, self.voices, voice_id, self.tick))
                process.start()

                return True

        print 'not found'

    def do_c(self, cmd):
        cmd = cmd.split(' ')
        t = cmd[0]
        cmd.pop(0)
        self.cc[t] = cmd

    def do_ss(self, cmd):
        gen = 'all'

        cmds = cmd.split(' ')
        if cmds[0] != '' and cmds[0] != 'all':
            gen = cmds

        for vid in range(1, self.vid + 1):
            if hasattr(self.voices, str(vid)):
                voice = getattr(self.voices, str(vid))

                if voice['gen'] in gen or gen == 'all':
                    voice['loop'] = False
                    setattr(self.voices, str(vid), voice)

    def do_s(self, cmd):
        cmds = cmd.split(' ')

        for cmd in cmds:
            vid = cmd.strip() 
            voice = getattr(self.voices, vid)
            voice['loop'] = False
            setattr(self.voices, vid, voice)

    def do_r(self, cmd):
        cmds = cmd.split(' ')

        for vid in range(1, self.vid + 1):
            if hasattr(self.voices, str(vid)):
                update = False
                voice = getattr(self.voices, str(vid))

                # loop through cmds and split
                for cmd in cmds:
                    cmd = cmd.split(':')
                    for c in voice['cmd']:
                        if cmd[0] == c[len(cmd[0])]:
                            update = True
                            voice['cmd'][i] = cmd[0] + ':' + cmd[1]

                if update is True:
                    voice['cmd'] += ['one']
                    setattr(self.voices, str(vid), voice)
                

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

    def do_i(self, cmd=[]):
        for vid in range(1, self.vid + 1):
            if hasattr(self.voices, str(vid)):
                voice = getattr(self.voices, str(vid))
                print vid, voice['gen'], [str(c) for c in voice['cmd']], 'v:', voice['tvol'], 'l:', voice['loop'], 're:', voice['regen']

    def do_p(self, cmd):
        if cmd in self.config['presets']:
            cmd = self.config['presets'][cmd]

            for c in cmd:
                if len(c) > 2:
                    self.default(c)
                    dsp.delay(0.1)

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
