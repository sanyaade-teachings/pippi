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
    logger.addHandler(SysLogHandler(address='/dev/log', facility=SysLogHandler.LOG_DAEMON))
    logger.setLevel(logging.DEBUG)
    warnings.simplefilter('always')


r = redis.StrictRedis(host='localhost', port=6379, db=0)
bus = r.pubsub()

PADMAP = {
    36: 'group1',
    37: 'group2',
    38: 'group3',
    39: 'group4',

    40: 'group5',
    41: 'group6',
    42: 'group7',
    43: 'group8',
}

def midi_relay(device_name, stop_event):
    with mido.open_input(device_name) as device:
        for msg in device:
            if stop_event.is_set():
                print('Stopping MIDI relay...')
                break

            if msg.type == 'note_on':
                #r.publish('astrid', 'play')
                #r.publish(PADMAP[msg.note], 'play')
                #print(PADMAP[msg.note], 'play')

                instrument = 'osc'
                params = 'note=%s' % msg.note

                try:
                    subprocess.run(['./build/qmessage', instrument, params])
                except Exception as e:
                    logger.error('Could not invoke qmessage: %s' % e)
                    logger.error(traceback.format_exc())


            elif msg.type == 'note_off':
                #r.publish('astrid', 'stop')
                #r.publish(PADMAP[msg.note], 'stop')
                #print(PADMAP[msg.note], 'stop')
                pass

            elif msg.type == 'control_change':
                r.publish('astrid', 'setval:cc%s:float:%s' % (msg.control, msg.value/128.))

            logger.info('MIDI message: %s' % msg)

class AstridConsole(cmd.Cmd):
    """ Astrid Console 
    """

    prompt = '^_- '
    intro = 'Astrid Console'
    instruments = {}
    dac = None
    adc = None
    midi_relay = None
    midi_stop_event = None

    def __init__(self, client=None):
        cmd.Cmd.__init__(self)
        self.midi_stop_event = threading.Event()

    def do_dac(self, cmd):
        if cmd == 'on' and self.dac is None:
            print('Starting dac...')
            self.dac = subprocess.Popen('./build/dac')
        elif cmd == 'off' and self.dac is not None:
            print('Stopping dac...')
            self.dac.terminate()

    def do_adc(self, cmd):
        if cmd == 'on' and self.adc is None:
            print('Starting adc...')
            self.adc = subprocess.Popen('./build/adc')
        elif cmd == 'off' and self.adc is not None:
            print('Stopping adc...')
            self.adc.terminate()

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
            print('MIDI Inputs:')
            for i, d in enumerate(mido.get_input_names()):
                print('%02d: %s' % (i, d))

        elif cmd.startswith('device'):
            try:
                _, device = cmd.split(' ')
                device = int(device)
            except Exception:
                device = 0
            self.midi_device = mido.get_input_names()[device]

        elif cmd == 'start':
            self.midi_relay = threading.Thread(target=midi_relay, args=(self.midi_device, self.midi_stop_event))
            self.midi_relay.start()        

        elif cmd == 'stop':
            if self.midi_relay is not None:
                self.midi_stop_event.set()
                self.midi_relay.join()
                self.midi_relay = None
                self.midi_stop_event.clear()

    def do_quit(self, cmd):
        self.quit()

    def do_EOF(self, line):
        return True

    def postloop(self):
        pass

    def start(self):
        self.cmdloop()

    def quit(self):
        print('Quitting')
        for instrument in self.instruments:
            #r.lpush('astrid-play-%s' % instrument, 'kill')
            self.instruments[instrument].terminate()

        if self.dac is not None:
            self.dac.terminate()

        if self.adc is not None:
            self.adc.terminate()

        self.midi_stop_event.set()
        self.midi_relay.join()

        exit(0)

if __name__ == '__main__':
    c = AstridConsole()

    try:
        c.start()
    except KeyboardInterrupt as e:
        c.quit()

