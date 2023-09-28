import cmd
import logging
from logging.handlers import SysLogHandler
import os
from pathlib import Path
import platform
from pprint import pprint
import random
import subprocess
import sys
import time
import tomllib
import traceback
import warnings

import redis
import rtmidi

from pippi import dsp


_redis = redis.StrictRedis(host='localhost', port=6379, db=0)

logger = logging.getLogger('astrid-console')
if not logger.handlers:
    if platform.system() == 'Darwin':
        log_path = '/var/run/syslog'
    else:
        log_path = '/dev/log'

    logger.addHandler(SysLogHandler(address=log_path))
    logger.setLevel(logging.DEBUG)
    warnings.simplefilter('always')

TRIGGERMAP_SECTION_TYPES = ('[midi]', '[messages]')

def parse_triggermapfile_section(maps: dict, section_type: str, section_lines: list):
    if section_type == '[midi]':
        maps['midi'] = {}
        for l in section_lines:
            print(section_type, l)
            k, v = tuple(l.split('='))
            if k == 'id':
                maps['midi']['device_id'] = v
            elif k == 'notes':
                if v == 'all':
                    maps['midi']['notes'] = list(range(21, 128))
                elif '-' in v:
                    nbeg, nend = tuple(notes.split('-'))
                    maps['midi']['notes'] = list(map(str, range(int(nbeg), int(nend)+1)))
                else:
                    maps['midi']['notes'] = [int(v)]
    elif section_type == '[messages]':
        maps['messages'] = []
        for msg in section_lines:
            maps['messages'] += [ msg.split() ]

    return maps

def load_triggermaps_from_file(path):
    maps = {}
    with open(path, 'r') as f:
        section_type = None
        section = []
        for line in f:
            line = line.strip()
            if line == '':
                continue

            if line in TRIGGERMAP_SECTION_TYPES:
                # Section headers trigger parsing of any previous section
                if section_type is not None:
                    maps = parse_triggermapfile_section(maps, section_type, section)

                # and start a new section
                section_type = line
                section = []

            else:
                # Everything else gets collected for parsing later
                section += [ line ]

        # Parse the final section
        if len(section) > 0:
            maps = parse_triggermapfile_section(maps, section_type, section)

    for note in maps['midi']['notes']:
        for msg in maps['messages']:
            note_params = ['note=%s' % note, 'midi_device=%s' % maps['midi']['device_id']]
            cmd = ['astrid-addnotemap', maps['midi']['device_id'], note] + msg + note_params
            subprocess.run(cmd)
            print(cmd)

class AstridConsole(cmd.Cmd):
    """ Astrid Console 
    """

    prompt = '^_- '
    intro = 'Astrid Console'
    instruments = {}
    serial_listeners = {}
    dacs = {}
    adc = None
    seq = None
    midi_relay = None
    midi_listener = None

    def __init__(self, sessionfile=None):
        cmd.Cmd.__init__(self)

        print('Starting seq...')
        self.seq = subprocess.Popen('astrid-seq')
        print('done!')

        _redis.set('mididevice'.encode('ascii'), '1'.encode('ascii'))

        # Load the session file if there is one to load
        if sessionfile is not None and Path('%s.toml' % sessionfile).exists():
            print('Loading %s session file...' % sessionfile)
            with open('%s.toml' % sessionfile, 'r') as f:
                cfgs = f.read()

            cfg = tomllib.loads(cfgs)
            for c in cfg['init']['commands']:
                self.onecmd(c)
            print('done!')

    def do_cue(self, name):
        with open('%s.toml' % name, 'r') as f:
            cfgs = f.read()

        cfg = tomllib.loads(cfgs)
        for cmd in cfg['init']['commands']:
            self.onecmd(cmd)

    def do_pause(self, seconds):
        time.sleep(float(seconds))

    def help_sound(self):
        txt = """
    ^_- sound device

Set the selected sound device to device 1

    ^_- sound device 1

Changing the default device requires the DAC/ADC 
to be restarted to take effect.
        """
        print(txt)

    def do_sound(self, cmd):
        if cmd.startswith('device'):
            try:
                _, device = tuple(cmd.split())
            except ValueError:
                print()
                print('Available audio devices')
                print()
                subprocess.run(['astrid-getdeviceids'])
                print()
                return

            subprocess.run(['astrid-setdeviceid', device])

    def do_dac(self, cmd):
        cmds = cmd.split()
        if len(cmds) == 1:
            dac_id = 0
            channels = 2

        elif len(cmds) == 2:
            dac_id = cmds[1]
            channels = 2

        elif len(cmds) == 3:
            dac_id = cmds[1]
            channels = cmds[2]

        else:
            print('Invalid cmd %s' % cmd)
            return

        if cmds[0] == 'on':
            if self.dacs.get(dac_id, None) is None:
                self.dacs[dac_id] = subprocess.Popen(['astrid-dac', str(dac_id), str(channels)])
            else:
                print('dac %s is already running' % dac_id)

        elif cmds[0] == 'off':
            if self.dacs.get(dac_id, None) is not None:
                self.dacs[dac_id].terminate()
                self.dacs[dac_id].wait()
                self.dacs[dac_id] = None
            else:
                print('dac %s is already stopped' % dac_id)

    def do_adc(self, cmd):
        if cmd == 'on':
            print('Starting adc...')
            if self.adc is None:
                self.adc = subprocess.Popen('astrid-adc')
            else:
                print('adc is already running')

        elif cmd == 'off':
            print('Stopping adc...')
            if self.adc is not None:
                self.adc.terminate()
                self.adc.wait()
                self.adc = None
            else:
                print('adc is already stopped')

    def do_seq(self, cmd):
        if cmd == 'on':
            print('Starting seq...')
            if self.seq is None:
                self.seq = subprocess.Popen('astrid-seq')
            else:
                print('seq is already running')

        elif cmd == 'off':
            print('Stopping seq...')
            if self.seq is not None:
                self.seq.terminate()
                self.seq.wait()
                self.seq = None
            else:
                print('seq is already stopped')

    def do_l(self, instrument):
        if instrument not in self.instruments:
            try:
                rcmd = 'astrid-renderer "orc/%s.py" "%s"' % (instrument, instrument)
                print(rcmd)
                self.instruments[instrument] = subprocess.Popen(rcmd, shell=True)
            except Exception as e:
                print('Could not start renderer: %s' % e)
                print(traceback.format_exc())
                return

    def do_t(self, cmd):
        parts = cmd.split(' ')
        instrument = parts[0]

        params = ''
        if len(parts) > 1:
            params = ' ' + ' '.join(parts)

        if instrument not in self.instruments:
            try:
                rcmd = 'astrid-renderer "orc/%s.py" "%s"' % (instrument, instrument)
                print(rcmd)
                self.instruments[instrument] = subprocess.Popen(rcmd, shell=True)
            except Exception as e:
                print('Could not start renderer: %s' % e)
                print(traceback.format_exc())
                return

        try:
            logger.info('Sending play msg to %s renderer w/params:\n  %s' % (instrument, params))
            subprocess.run(['astrid-qmessage', 't', instrument, params])
        except Exception as e:
            print('Could not invoke qmessage: %s' % e)
            print(traceback.format_exc())

    def do_cb(self, chords):
        _redis.set('chordbank'.encode('ascii'), chords.encode('ascii'))
        print('Set chord bank to %s' % chords)

    def do_tm(self, cmd):
        """ Manage trigger maps
        """
        parts = [ p.strip() for p in cmd.split() ]

        if parts[0] == 'load':
            load_triggermaps_from_file(parts[1])
            return

        action = parts.pop(0)
        device = parts.pop(0)
        notes = parts.pop(0)

        # FIXME device IDs should be optional or runtime configurable
        #devices = ['1','2','3']

        if '-' in notes:
            nbeg, nend = tuple(notes.split('-'))
            notes = list(map(str, range(int(nbeg), int(nend)+1)))
        elif notes == 'all':
            notes = list(map(str, range(21, 128)))
        else:
            notes = [notes]

        if action == 'a':
            for note in notes:
                note_params = ['note=%s' % note, 'midi_device=%s' % device]
                subprocess.run(['astrid-addnotemap', device, note] + parts + note_params)
                print('Added notemap for device %s note %s cmd %s' % (device, note, parts + note_params))

        elif action == 'c':
            for note in notes:
                try:
                    os.unlink('/tmp/astrid-midimap-device%s-note%s' % (device, note))
                    print('Removed all notemaps for device %s note %s' % (device, note))
                except FileNotFoundError as e:
                    pass

        elif action == 'l':
            for note in notes:
                subprocess.run(['astrid-printnotemap', device, note])

    def do_p(self, cmd):
        parts = cmd.split(' ')
        instrument = parts[0]

        params = ''
        if len(parts) > 1:
            params = ' ' + ' '.join(parts)

        if instrument not in self.instruments:
            try:
                rcmd = 'astrid-renderer "orc/%s.py" "%s"' % (instrument, instrument)
                print(rcmd)
                self.instruments[instrument] = subprocess.Popen(rcmd, shell=True)
            except Exception as e:
                print('Could not start renderer: %s' % e)
                print(traceback.format_exc())
                return

        try:
            logger.info('Sending play msg to %s renderer w/params:\n  %s' % (instrument, params))
            subprocess.run(['astrid-qmessage', 'p', instrument, params])
        except Exception as e:
            print('Could not invoke qmessage: %s' % e)
            print(traceback.format_exc())

    def do_set(self, cmd):
        parts = [ p.strip() for p in cmd.split() ]
        k = parts[0]
        v = ' '.join(parts[1:])
        _redis.set(k.encode('ascii'), v.encode('ascii'))

    def do_setid(self, cmd):
        try:
            name, value = tuple(cmd.split(' '))
            value = int(value)
        except Exception as e:
            print('Bad command. Naughty!')
            print(traceback.format_exc())
            return

        try:
            logger.info('Setting value %s to %s' % (name, value))
            subprocess.run(['astrid-qmessage', 'v', name, value])
        except Exception as e:
            print('Could not set value with qmessage: %s' % e)
            print(traceback.format_exc())

    def do_i(self, cmd):
        print(self.instruments)

    def do_s(self, voice_id):
        try:
            logger.info('Sending stop msg to voice %s\n' % voice_id)
            subprocess.run(['astrid-qmessage', 's', voice_id])
        except Exception as e:
            print('Could not invoke qmessage: %s' % e)
            print(traceback.format_exc())

    def do_k(self, instrument):
        if instrument in self.instruments:
            self.instruments[instrument].terminate()
            self.instruments[instrument].wait()
            del self.instruments[instrument]

    def do_serial(self, params):
        cmd, tty = tuple([ p.strip() for p in params.split() ])

        if cmd == 'on':
            print('Starting serial listener...')
            if tty in self.serial_listeners and self.serial_listeners[tty] is None:
                self.serial_listeners[tty] = subprocess.Popen('astrid-seriallistener /dev/%s' % tty)
                print('Serial listener %s has started' % tty)
            else:
                print('Serial listener %s is already on' % tty)

        elif cmd == 'off':
            if tty in self.serial_listeners and self.serial_listeners[tty] is None:
                print('Stopping serial listener...')
                self.serial_listeners[tty].terminate()
                self.serial_listeners[tty].wait()
                del self.serial_listeners[tty]
                print('Serial listener has stopped')
            else:
                print('Serial listener is already off')

        elif cmd == 'list' or cmd == '':
            pass

    def help_midi(self):
        txt = """
Start the MIDI listener (input) and relay (output) threads

    ^_- midi on

Stop them again

    ^_- midi off

List all connected MIDI devices

    ^_- midi list

The currently selected default device will be 
displayed in \033[1mbold text\033[22m.

This works too:

    ^_- midi

List connected output devices

    ^_- midi out

List connected input devices

    ^_- midi in

Set the default MIDI output device to device 1

    ^_- midi out 1

Set the default MIDI input device to device 1

    ^_- midi in 1
"""
        print(txt)

    def do_midi(self, cmd):
        midiout = rtmidi.MidiOut()
        midiin = rtmidi.MidiIn()
        inputs = midiin.get_ports()
        outputs = midiout.get_ports()

        if cmd == 'on':
            print('Starting MIDI relay...')
            if self.midi_relay is None:
                self.midi_relay = subprocess.Popen('/home/hecanjog/code/pippi/astrid/python/midiseq.py')
                print('MIDI relay has started')
            else:
                print('MIDI relay is already on')

            print('Starting MIDI listener...')
            if self.midi_listener is None:
                self.midi_listener = subprocess.Popen('/home/hecanjog/code/pippi/astrid/python/midistatus.py')
                print('MIDI listener has started')
            else:
                print('MIDI listener is already on')

        elif cmd == 'off':
            if self.midi_relay is not None:
                print('Stopping MIDI relay...')
                self.midi_relay.terminate()
                self.midi_relay.wait()
                self.midi_relay = None
                print('MIDI relay has stopped')
            else:
                print('MIDI relay is already off')

            if self.midi_listener is not None:
                print('Stopping MIDI listener...')
                self.midi_listener.terminate()
                self.midi_listener.wait()
                self.midi_listener = None
                print('MIDI listener has stopped')
            else:
                print('MIDI listener is already off')

        elif cmd == 'list' or cmd == '':
            self.list_midiin_devices(inputs)
            self.list_midiout_devices(outputs)

        elif cmd.startswith('out'):
            try:
                _, device = cmd.split()
                device = int(device)
                self.set_default_midiout_device(device, outputs[device])
            except Exception:
                self.list_midiout_devices(outputs)

        elif cmd.startswith('in'):
            try:
                _, device = cmd.split(' ')
                device = int(device)
                self.set_default_midiin_device(device, inputs[device])
            except Exception:
                self.list_midiin_devices(inputs)

    def print_mididevice(self, index: int, name: str, selected: int):
        if index == selected:
            print('\033[1m%d - %s\033[22m' % (index, name))
        else:
            print('%d - %s' % (index, name))

    def list_midiout_devices(self, outputs):
        default_out = self.get_default_midiout_device()
        print('MIDI Outputs (%d)' % len(outputs))
        for i, d in enumerate(outputs):
            self.print_mididevice(i, d, default_out)

    def list_midiin_devices(self, inputs):
        default_in = self.get_default_midiin_device()
        print('MIDI Inputs (%d)' % len(inputs))
        for i, d in enumerate(inputs):
            self.print_mididevice(i, d, default_in)

    def get_default_midiin_device(self) -> int:
        try:
            with open('/tmp/astrid-default-midiin-device', 'r') as f:
                device = f.read()
            device = int(device)
        except FileNotFoundError:
            device = 0

        return device

    def get_default_midiout_device(self) -> int:
        try:
            with open('/tmp/astrid-default-midiout-device', 'r') as f:
                device = f.read()
            device = int(device)
        except FileNotFoundError:
            device = 0

        return device

    def set_default_midiin_device(self, index: int, name: str):
        with open('/tmp/astrid-default-midiin-device', 'w') as f:
            f.write('%d' % index)

    def set_default_midiout_device(self, index: int, name: str):
        with open('/tmp/astrid-default-midiout-device', 'w') as f:
            f.write('%d' % index)

    def do_quit(self, cmd):
        self.quit()

    def do_EOF(self, line):
        return True

    def postloop(self):
        pass

    def start(self):
        self.cmdloop()

    def quit(self):
        print('Shutting down...')
        for instrument in self.instruments:
            print('Stopping renderer for %s...' % instrument)
            self.instruments[instrument].terminate()
            self.instruments[instrument].wait()
            del self.instruments[instrument]

        if self.dac is not None:
            print('Stopping DAC mixer...')
            self.dac.terminate()
            self.dac.wait()

        if self.adc is not None:
            print('Stopping ADC ringbuf recorder...')
            self.adc.terminate()
            self.dac.wait()

        print('All done. Bye-bye!')
        exit(0)

if __name__ == '__main__':
    sessionfile = None
    if len(sys.argv) > 1:
        sessionfile = sys.argv[1]

    c = AstridConsole(sessionfile)

    try:
        c.start()
    except KeyboardInterrupt as e:
        c.quit()

