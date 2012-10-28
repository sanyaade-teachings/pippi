import cmd
import os
import sys
import math
import dsp
import alsaaudio
import time
import json
import param
import rt
import multiprocessing as mp
from termcolor import colored

class Pippi(cmd.Cmd):
    """ Pippi Console 
    """

    prompt = 'pippi: '
    intro = 'Pippi Console'

    cc = {} 
    vid = 0

    def __init__(self):
        cmd.Cmd.__init__(self)

        # TODO: recursively merge config files
        # Allow for reloading / loading from console
        config = open('config/global.json')
        self.config = json.load(config)

        config = open('config/suite.json')
        self.config.update(json.load(config))

        self.bpm = self.config['bpm']

        self.server = mp.Manager()
        self.voices = self.server.Namespace()

        self.voice_id = 0

        self.tick = mp.Event()

        self.grid = mp.Process(target=rt.grid, args=(self.tick, self.bpm))
        self.grid.start()

    def play(self, generator, params):
        # Generator scripts expect a simple dict in 
        #   { param: value } 
        # format. Type conversions and range checks should 
        # always be done before being passed into the generator.
        #
        # Malkovitch Malkovitch param param
        params.bpm = self.bpm # omfg noooo
        params = { param: params.get(param) for (param, value) in params.data.iteritems() }

        if 'bpm' not in params:
            params['bpm'] = self.bpm

        try:
            # Increment voice id and print voice information. TODO: pretty print & abstract
            self.voice_id += 1
            print self.voice_id, self.format_params(params)

            # Allocate a new shared voice dict to store generator params, and audio
            # data for the render process. (Render processes are spawned on demand by 
            # the voice's playback process.)
            #
            # Voices are stored in a shared namespace (self.voices) and keyed by id.

            voice = {'snd': '', 'next': '', 'loop': True, 'tvol': 1.0, 'params': params}
            setattr(self.voices, str(self.voice_id), voice)

            # Import the generator as a python module and spawn a playback 
            # process from rt.out, passing in the generator's play method 
            # which will be run from within a render process - spawned on demand 
            # by the voice's playback process. Sheesh.
            #
            # Generator scripts are stored in the 'orc' directory for now.

            generator = 'orc.' + generator
            generator = __import__(generator, globals(), locals(), ['play'])
            playback_process = mp.Process(target=rt.out, args=(generator.play, self.voices, self.voice_id, self.tick))
            playback_process.start()

        except ImportError:
            # We'll get an ImportError exception here if a generator has been registered 
            # in the json config file and there is no matching generator script in the orc/ directory.

            print 'invalid generator'

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

        for vid in range(1, self.voice_id + 1):
            if hasattr(self.voices, str(vid)):
                voice = getattr(self.voices, str(vid))

                if voice['params']['generator'] in gen or gen == 'all':
                    voice['loop'] = False
                    setattr(self.voices, str(vid), voice)

    def do_s(self, cmd):
        cmds = cmd.split(' ')

        for cmd in cmds:
            vid = cmd.strip() 
            if hasattr(self.voices, vid):
                voice = getattr(self.voices, vid)
                voice['loop'] = False
                setattr(self.voices, vid, voice)

    def do_vv(self, cmd):
        cmds = cmd.split(' ')

        for vid in range(1, self.voice_id + 1):
            self.setvol(vid, cmds[1], cmds[0])

    def do_v(self, cmd):
        cmds = cmd.split(' ')
        self.setvol(cmds[0], cmds[1])

    def setvol(self, vid, vol, gen='a'):
        if hasattr(self.voices, str(vid)):
            voice = getattr(self.voices, str(vid))
            if gen == voice['params']['generator'] or gen == 'a':
                voice['tvol'] = float(vol) / 100.0
                setattr(self.voices, str(vid), voice)

    def do_i(self, cmd=[]):
        for vid in range(1, self.voice_id + 1):
            if hasattr(self.voices, str(vid)):
                voice = getattr(self.voices, str(vid))
                print vid, self.format_params(voice['params']), 'v:', voice['tvol'], 'loop:', voice['loop']

    def format_params(self, params=[]):
        # TODO: translate types & better formatting
        param_string = ''
        for param in params:
            param_string += colored(str(param)[0:3] + ': ', 'cyan') + colored(str(params[param]), 'yellow') + ' '
        
        return param_string

    def do_p(self, cmd):
        if cmd in self.config['presets']:
            cmd = self.config['presets'][cmd]

            for c in cmd:
                if len(c) > 2:
                    self.default(c)
                    dsp.delay(0.1)

    def default(self, cmd):
        # Break comma separated commands
        # into a list of command strings
        cmds = cmd.strip().split(',')

        # For each command string, unpack and load
        # into a param.Param instance based on 
        # current json config rules. 
        #
        # So:
        #   sh o:3
        #
        # Could become:
        #   { 'generator': 'shine', 'octave': {'value': 3, 'type': 'integer'} }
        # 
        # And:
        #   dr h:1.2.3.4 t:4b wf:tri n:eb.g
        # 
        # Could become:
        #   {
        #       'generator': 'drone',
        #       'harmonics': {'value': '1.2.3.4', 'type': 'integer-list'},
        #       'length': {'value': '4b', 'type': 'frame'},
        #       'waveform': {'value': 'tri'},
        #       'notes': {'value': 'eb.g', 'type': 'note-list'}
        #   }
        # 
        # For a complete list of reserved words and built-in types please 
        # refer to the documentation I haven't written yet. Oh, bummer.
        # In the meantime, the patterns in param.py should help a little.
        #
        # Finally pass to self.play() to spawn render and playback processes.

        for cmd in cmds:
            params = param.unpack(cmd, self.config)
            generator = params.get('generator')

            if generator is False:
                print 'invalid generator'
            else:
                self.play(generator, params)

    def do_EOF(self, line):
        return True

    def postloop(self):
        pass

if __name__ == '__main__':
        # Create console
        console = Pippi()

        # Start looping command prompt
        console.cmdloop()
