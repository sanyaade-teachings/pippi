import cmd
import threading
import traceback
import subprocess

import mido
import redis

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

def midi_relay(self, *args, **kwargs):
    with mido.open_input('LPD8:LPD8 MIDI 1 20:0') as lpd:
        for msg in lpd:
            if msg.type == 'note_on':
                #r.publish('astrid', 'play')
                r.publish(PADMAP[msg.note], 'play')
                print(PADMAP[msg.note], 'play')
            elif msg.type == 'note_off':
                #r.publish('astrid', 'stop')
                r.publish(PADMAP[msg.note], 'stop')
                print(PADMAP[msg.note], 'stop')
            elif msg.type == 'control_change':
                r.publish('astrid', 'setval:cc%s:float:%s' % (msg.control, msg.value/128.))

            print(msg)

class AstridConsole(cmd.Cmd):
    """ Astrid Console 
    """

    prompt = '^_- '
    intro = 'Astrid Console'
    instruments = {}
    dac = None
    adc = None

    def __init__(self, client=None):
        cmd.Cmd.__init__(self)

    def do_dac(self, cmd):
        if cmd == 'on' and self.dac is None:
            print('Starting dac...')
            self.dac = subprocess.Popen('./build/dac')
        elif cmd == 'off' and self.dac is not None:
            print('Stopping dac...')
            #self.dac.kill()
            self.dac.terminate()

    def do_adc(self, cmd):
        if cmd == 'on' and self.adc is None:
            print('Starting adc...')
            self.adc = subprocess.Popen('./build/adc')
        elif cmd == 'off' and self.adc is not None:
            print('Stopping adc...')
            #self.adc.kill()
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
        #if instrument in self.instruments:
        #    r.lpush('astrid-play-%s' % instrument, 'kill')
        #    self.instruments[instrument].terminate()
        pass

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

        exit(0)

if __name__ == '__main__':
    c = AstridConsole()
    #mr = threading.Thread(target=midi_relay, args=(None,))

    try:
        #mr.start()        
        c.start()
    except KeyboardInterrupt as e:
        c.quit()

    #mr.join()
