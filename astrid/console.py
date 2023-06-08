import cmd
import logging
from logging.handlers import SysLogHandler
import platform
import subprocess
import traceback
import warnings

import mido
import rtmidi


logger = logging.getLogger('astrid-console')
if not logger.handlers:
    if platform.system() == 'Darwin':
        log_path = '/var/run/syslog'
    else:
        log_path = '/dev/log'

    logger.addHandler(SysLogHandler(address=log_path))
    logger.setLevel(logging.DEBUG)
    warnings.simplefilter('always')


class AstridConsole(cmd.Cmd):
    """ Astrid Console 
    """

    prompt = '^_- '
    intro = 'Astrid Console'
    instruments = {}
    dac = None
    adc = None
    midi_relay = None
    midi_listener = None

    def __init__(self, client=None):
        cmd.Cmd.__init__(self)

    def help_sound(self):
        txt = """
Turn sound on (Starts the DAC & ADC)

    ^_- sound on

Turn sound off (Stops the DAC & ADC)

    ^_- sound off

List available sound devices

    ^_- sound device

Set the selected sound device to device 1

    ^_- sound device 1

Changing the default device requires the DAC/ADC 
to be restarted to take effect.
        """
        print(txt)

    def do_sound(self, cmd):
        if cmd == 'on' and self.dac is None:
            print('Starting dac & adc...')
            if self.dac is None:
                self.dac = subprocess.Popen('./build/astrid-dac')
            else:
                print('dac is already running')

            if self.adc is None:
                self.adc = subprocess.Popen('./build/astrid-adc')
            else:
                print('adc is already running')

        elif cmd == 'off':
            if self.dac is not None:
                print('Stopping dac...')
                self.dac.terminate()
                self.dac.wait()
                self.dac = None
            else:
                print('dac is already stopped')

            if self.adc is not None:
                print('Stopping adc...')
                self.adc.terminate()
                self.adc.wait()
                self.adc = None
            else:
                print('adc is already stopped')

        elif cmd.startswith('device'):
            try:
                _, device = cmd.split(' ')
            except ValueError:
                print()
                print('Available audio devices')
                print()
                subprocess.run(['./build/astrid-getdeviceids'])
                print()
                return

            subprocess.run(['./build/astrid-setdeviceid', device])

    def do_dac(self, cmd):
        if cmd == 'on' and self.dac is None:
            print('Starting dac...')
            if self.dac is None:
                self.dac = subprocess.Popen('./build/astrid-dac')
            else:
                print('dac is already running')

        elif cmd == 'off' and self.dac is not None:
            print('Stopping dac...')
            if self.dac is not None:
                self.dac.terminate()
                self.dac.wait()
                self.dac = None
            else:
                print('dac is already stopped')

    def do_adc(self, cmd):
        if cmd == 'on' and self.adc is None:
            print('Starting adc...')
            if self.adc is None:
                self.adc = subprocess.Popen('./build/astrid-adc')
            else:
                print('adc is already running')

        elif cmd == 'off' and self.adc is not None:
            print('Stopping adc...')
            if self.adc is not None:
                self.adc.terminate()
                self.adc.wait()
                self.adc = None
            else:
                print('adc is already stopped')

    def do_l(self, instrument):
        if instrument not in self.instruments:
            try:
                rcmd = './build/astrid-renderer "orc/%s.py" "%s"' % (instrument, instrument)
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
                rcmd = './build/astrid-renderer "orc/%s.py" "%s"' % (instrument, instrument)
                print(rcmd)
                self.instruments[instrument] = subprocess.Popen(rcmd, shell=True)
            except Exception as e:
                print('Could not start renderer: %s' % e)
                print(traceback.format_exc())
                return

        try:
            logger.info('Sending play msg to %s renderer w/params:\n  %s' % (instrument, params))
            subprocess.run(['./build/astrid-qmessage', 't', instrument, params])
        except Exception as e:
            print('Could not invoke qmessage: %s' % e)
            print(traceback.format_exc())

    def do_p(self, cmd):
        parts = cmd.split(' ')
        instrument = parts[0]

        params = ''
        if len(parts) > 1:
            params = ' ' + ' '.join(parts)

        if instrument not in self.instruments:
            try:
                rcmd = './build/astrid-renderer "orc/%s.py" "%s"' % (instrument, instrument)
                print(rcmd)
                self.instruments[instrument] = subprocess.Popen(rcmd, shell=True)
            except Exception as e:
                print('Could not start renderer: %s' % e)
                print(traceback.format_exc())
                return

        try:
            logger.info('Sending play msg to %s renderer w/params:\n  %s' % (instrument, params))
            subprocess.run(['./build/astrid-qmessage', 'p', instrument, params])
        except Exception as e:
            print('Could not invoke qmessage: %s' % e)
            print(traceback.format_exc())

    def do_v(self, cmd):
        k, v = tuple(cmd.split('='))
        r.set(k.encode('ascii'), v.encode('ascii'))

    def do_i(self, cmd):
        print(self.instruments)

    def do_s(self, instrument):
        #if instrument in self.instruments:
        #    r.lpush('astrid-play-%s' % instrument, 'stop')
        pass

    def do_k(self, instrument):
        if instrument in self.instruments:
            #r.lpush('astrid-play-%s' % instrument, 'kill')
            self.instruments[instrument].terminate()

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

        if cmd == 'on' and self.dac is None:
            print('Starting MIDI relay...')
            if self.midi_relay is None:
                self.midi_relay = subprocess.Popen('./python/midiseq.py')
                print('MIDI relay has started')
            else:
                print('MIDI relay is already on')

            print('Starting MIDI listener...')
            if self.midi_listener is None:
                self.midi_listener = subprocess.Popen('./python/midistatus.py')
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
    c = AstridConsole()

    try:
        c.start()
    except KeyboardInterrupt as e:
        c.quit()

