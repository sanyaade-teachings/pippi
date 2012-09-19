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
        self.data = self.server.Namespace()
        self.data.p = {}
        self.data.v = {}
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
            bpm = (dsp.bpm2ms(72.0) / 1000.0) * 0.25
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


    def render(self, play, args, vid, data, label='snd'):
        d = data.v
        d[vid][label] = dsp.split(play(args), 500)
        data.v = d

    def out(self, play, args, gen, data, vid, qu):
        """
            Worker playback process spawned by play()
            Manages render threads for voices and render buffers for playback
        """

        # Render sound
        render = mp.Process(target=self.render, args=(play, args, vid, data))
        render.start()
        render.join()

        # Open a connection to an ALSA PCM device
        # Split buffered sound into packets and push to playback
        if gen == 'sp' or gen == 'sh':
            device = 'T6_pair1'
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

        for arg in args:
            if arg == 're':
                regen = True

            if arg[:2] == 'qu':
                q = int(arg[3:].strip())

        # See if we have a sound ready
       
        vol = 1.0
        tvol = 1.0
        
        v = data.v
        while v[vid]['loop'] == True:
            v = data.v
            if regen == True and cooking == False:
                cooking = True
                next = mp.Process(target=self.render, args=(play, args, vid, data, 'next'))
                next.start()

            if next is not False and next.is_alive() == False:
                cooking = False
                snd = v[vid]['next']
            else:
                snd = v[vid]['snd']

            if q is not False:
                qu[q].wait()

            for s in snd:
                out.write(s)

        # Remove self from playback info before terminating the process
        p = self.data.p
        p.pop(vid)
        data.p = p


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

                # Create a shared list buffer for the voice
                self.vid += 1
                data = self.data.v
                data[self.vid] = {'snd': '', 'next': '', 'loop': True, 'regen': False}
                self.data.v = data

                process = mp.Process(target=self.out, args=(p.play, cmd, t, self.data, self.vid, self.qu))
                process.start()

                p = self.data.p
                p[self.vid] = {'t': t, 'cmd': cmd}
                self.data.p = p

                return True

        print 'not found'

    def do_c(self, cmd):
        cmd = cmd.split(' ')
        t = cmd[0]
        cmd.pop(0)
        self.cc[t] = cmd

    def do_ss(self, cmd):
        v = self.data.v
        p = self.data.p
        for s in p:
            v[s]['loop'] = False

        self.data.v = v

    def do_s(self, cmd):
        cmds = cmd.split(' ')

        try:
            v = self.data.v
            for cmd in cmds:
                v[int(cmd)]['loop'] = False
            self.data.v = v

        except KeyError:
            print 'nope'

    # TODO implement volume control again
    #def do_vv(self, cmd):
        #cmds = cmd.split(' ')

        #for p in self.playing:
            #if self.playing[p]['t'] == cmds[0] or cmds[0] == 'a':
                #vol = float(cmds[1]) / 100.0
                #self.playing[p]['q'].put({'vol': vol})
                #self.playing[p]['vol'] = vol


    #def do_v(self, cmd):
        #cmds = cmd.split(' ')
        #try:
            #vol = float(cmds[1]) / 100.0
            #i = int(cmds[0])
            #self.playing[i]['q'].put({'vol': vol})
            #self.playing[i]['vol'] = vol
        #except KeyError:
            #print 'nope, nope'
        #except IndexError:
            #print 'nope'

    def do_i(self, cmd=[]):
        try:
            pp = self.data.p
            for p in pp:
                print p, pp[p]['t'], pp[p]['cmd']

        except KeyError:
            print 'nope'

    def do_p(self, cmd):
        if cmd == 't':
            cmd = [
                    'cl d:k regen bpm:130 v:40',
                    ]

        for c in cmd:
            if len(c) > 2:
                self.default(c)

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
