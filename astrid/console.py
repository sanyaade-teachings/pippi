import cmd
import logging
from logging.handlers import SysLogHandler
import subprocess
import threading
import traceback
import warnings

import mido
import redis


logger = logging.getLogger('astrid-console')
if not logger.handlers:
    logger.addHandler(FileHandler('/tmp/astrid.log', encoding='utf-8'))
    logger.setLevel(logging.DEBUG)
    warnings.simplefilter('always')


r = redis.StrictRedis(host='localhost', port=6379, db=0)
bus = r.pubsub()


def midi_relay(device_name, stop_event):
    waiter = threading.Event()
    logger.info('Starting MIDI relay for device %s...' % device_name)
    with mido.open_input(device_name) as device:
        registered_instruments = r.hkeys('%s-triggers' % device_name)
        while True:
            if stop_event.is_set():
                logger.info('Got stop event for %s MIDI listener' % device_name)
                break

            if device.closed:
                logger.info('Stopping %s MIDI listener: port has been closed' % device_name)
                break

            for msg in device.iter_pending():
                if msg.type == 'note_on':
                    r.set('%s-note%03d' % (device_name, msg.note), msg.velocity)

                    logger.debug('Registered instruments for device %s:\n    %s' % (device_name, registered_instruments))
                    registered_instruments = ['osc']

                    for instrument_name in registered_instruments:
                        lowval = r.get('%s-%s-triglow' % (device_name, instrument_name))
                        logger.debug('Low trigger val: %s' % lowval)
                        lowval = int(lowval or 0)
                        if lowval is None or msg.note < lowval:
                            break

                        highval = r.get('%s-%s-trighigh' % (device_name, instrument_name))
                        highval = int(highval or 127)
                        logger.debug('High trigger val: %s' % highval)
                        if highval is None or msg.note > highval:
                            break

                        params = 'note=%s velocity=%s' % (msg.note, msg.velocity)

                        try:
                            subprocess.run(['./build/qmessage', instrument_name, params])
                        except Exception as e:
                            logger.exception('Could not invoke qmessage: %s' % e)

                elif msg.type == 'note_off':
                    r.set('%s-note%03d' % (device_name, msg.note), 0)

                elif msg.type == 'control_change':
                    r.set('%s-cc%03d' % (device_name, msg.control), msg.value)

                logger.debug('MIDI message: %s' % msg)

            # update triggers for this device
            # triggers are a dict where 
            #   keys are device names
            #   values are a dict where:
            #     keys are instrument names
            #     values are a note range tuple: low val, high val
            # on each loop: 
            #   look up the trigger dict for this device
            #   copy the instrument name to note range mappings to local dict
            #   use the dict to look up triggers
            registered_instruments = r.hkeys('%s-triggers' % device_name)
            waiter.wait(timeout=1)

    logger.info('Stopping MIDI relay for device %s...' % device_name)


class AstridConsole(cmd.Cmd):
    """ Astrid Console 
    """

    prompt = '^_- '
    intro = 'Astrid Console'
    instruments = {}
    dac = None
    adc = None
    midi_relays = {}

    def __init__(self, client=None):
        cmd.Cmd.__init__(self)
        self.midi_stop_event = threading.Event()

    def do_sound(self, cmd):
        if cmd == 'on' and self.dac is None:
            print('Starting dac & adc...')
            if self.dac is None:
                self.dac = subprocess.Popen('./build/dac')
            else:
                print('dac is already running')

            if self.adc is None:
                self.adc = subprocess.Popen('./build/adc')
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

    def do_dac(self, cmd):
        if cmd == 'on' and self.dac is None:
            print('Starting dac...')
            if self.dac is None:
                self.dac = subprocess.Popen('./build/dac')
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
                self.adc = subprocess.Popen('./build/adc')
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
                rcmd = 'INSTRUMENT_PATH="orc/%s.py" INSTRUMENT_NAME="%s" ./build/renderer' % (instrument, instrument)
                self.instruments[instrument] = subprocess.Popen(rcmd, shell=True)
            except Exception as e:
                print('Could not start renderer: %s' % e)
                print(traceback.format_exc())
                return

    def do_p(self, cmd):
        parts = cmd.split(' ')
        instrument = parts[0]

        params = ''
        if len(parts) > 1:
            params = ' ' + ' '.join(parts)

        if instrument not in self.instruments:
            try:
                rcmd = 'INSTRUMENT_PATH="orc/%s.py" INSTRUMENT_NAME="%s" ./build/renderer' % (instrument, instrument)
                self.instruments[instrument] = subprocess.Popen(rcmd, shell=True)
            except Exception as e:
                print('Could not start renderer: %s' % e)
                print(traceback.format_exc())
                return

        try:
            logger.info('Sending play msg to %s renderer w/params:\n  %s' % (instrument, params))
            subprocess.run(['./build/qmessage', instrument, params])
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

    def do_midi(self, cmd):
        if cmd == 'list':
            inputs = mido.get_input_names()
            print('Found %d MIDI inputs:' % len(inputs))
            for i, d in enumerate(inputs):
                l = '[LISTENING]' if d in self.midi_relays else ''
                print('%02d: %s %s' % (i, d, l))

        elif cmd.startswith('device'):
            try:
                _, device = cmd.split(' ')
                device = int(device)
            except Exception:
                device = 0
            self.default_midi_device = mido.get_input_names()[device]

        elif cmd.startswith('start'):
            try:
                _, device_id = cmd.split(' ')
                device = mido.get_input_names()[int(device_id)]
            except Exception:
                device = self.default_midi_device or mido.get_input_names()[0]

            if device in self.midi_relays:
                print('MIDI relay for %s is already started' % device)
                return

            stop_event = threading.Event()
            self.midi_relays[device] = (
                stop_event,
                threading.Thread(target=midi_relay, args=(device, stop_event))
            )
            self.midi_relays[device][1].start()        
            print('Started MIDI relay for %s' % device)

        elif cmd.startswith('stop'):
            try:
                _, device_id = cmd.split(' ')
                device = mido.get_input_names()[int(device_id)]
            except Exception:
                device = self.default_midi_device or mido.get_input_names()[0]

            if not device in self.midi_relays:
                print('No MIDI relay started for device %s' % device)
                return
            
            self.midi_relays[device][0].set()
            self.midi_relays[device][1].join()
            del self.midi_relays[device]

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

        for device_name, mr in self.midi_relays.items():
            print('Stopping MIDI relay for %s...' % device_name)
            mr[0].set()
            mr[1].join()

        print('All done. Bye-bye!')
        exit(0)

if __name__ == '__main__':
    c = AstridConsole()

    try:
        c.start()
    except KeyboardInterrupt as e:
        c.quit()

