import mido
import multiprocessing as mp
import dsp

def list_devices():
    return {
        'input': mido.get_input_names(),
        'output': mido.get_output_names(),
    }

def validate_output_device(device):
    return device in list_devices()['output']

def validate_input_device(device):
    return device in list_devices()['output']

def print_devices():
    devices = list_devices()

    print 'Input devices:'
    for device in devices['input']:
        print '  ' + device

    print
    print 'Output devices:'
    for device in devices['output']:
        print '  ' + device

def input_log(ns, active=True):
    def log_listener():
        device_names = mido.get_input_names()
        devices = [ mido.open_input(device_name) for device_name in device_names ]
        devices = mido.ports.MultiPort(devices)
        for msg in devices:
            if hasattr(ns, 'midi_log_active'):
                dsp.log(msg)
            else:
                continue

    if active:
        if not hasattr(ns, 'midi_log_active'):
            listener = mp.Process(target=log_listener)
            listener.start()
            setattr(ns, 'midi_log_active', True)
    else:
        delattr(ns, 'midi_log_active')

def device_scribe(device_name, ns):
    with mido.open_input(device_name) as incoming:
        for msg in incoming:
            msg_id = None
            value = None
            if hasattr(msg, 'control'):
                msg_id = msg.control
                value = msg.value

            if hasattr(msg, 'note'):
                msg_id = msg.note
                value = msg.velocity
                
            setattr(ns, '%s-%s-%s' % (device_name, msg.type, msg_id), value)

    delattr(ns, '%s-listener' % device_name)

def register_midi_listener(device_name, ns):
    # check to see if this device has a listener already
    # start one up if not
    if not hasattr(ns, '%s-listener' % device_name):
        try:
            listener = mp.Process(target=device_scribe, args=(device_name, ns))
            listener.start()
            setattr(ns, '%s-listener' % device_name, listener.pid)
        except IOError:
            dsp.log('Could not start listener for unknown MIDI device: %s' % device_name)

def get_midi_readers(devices, mappings, ns):
    readers = {}
    for shortname, device_name in devices.iteritems():
        register_midi_listener(device_name, ns)
        readers[shortname] = MidiReader(device_name, ns, mappings=mappings)

    return readers

class MidiTrigger:
    def __init__(self, device_name, notes):
        self.device_name = device_name
        self.notes = notes

    def wait(self):
        with mido.open_input(self.device_name) as incoming:
            for msg in incoming:
                if msg.type == 'note_on' and msg.note in self.notes:
                    return (msg.note, msg.velocity)

        return (None, None)

class MidiWriter:
    def __init__(self):
        pass

class MidiReader:
    def __init__(self, device_name, ns, offset=None, mappings=None):
        self.device_name = device_name
        self.ns = ns
        self.offset = offset
        self.mappings = mappings

    def setOffset(self, offset):
        self.offset = offset

    def geti(self, cc, default=None, low=0, high=1):
        return int(round(self.get(cc, default, low, high)))

    def getr(self, cc, default=None, low=0, high=1, mul=1):
        mul = 1 if mul > 1 else mul
        mul = dsp.rand(0, mul)

        value = self.get(cc, default, low, high)

        return value * mul

    def getri(self, cc, default=None, low=0, high=1, spread=1):
        return int(round(self.getr(cc, default, low, high, spread)))

    def get(self, cc, default=None, low=0, high=1):
        if self.mappings is not None and cc in self.mappings:
            cc = self.mappings[cc]

        if self.offset is not None:
            cc = cc + self.offset

        try:
            value = getattr(self.ns, '%s-control_change-%s' % (self.device_name, cc))
            return (value / 127.0) * (high - low) + low
        except Exception:
            if default is None:
                default = high 

            return default

    def note(self, midi_note, default=None, low=0, high=1):
        """ NOTE for note: this will just fetch note_on events 
            TODO: do we ever need to care about note_off here? 
        """

        try:
            velocity = getattr(self.ns, '%s-note_on-%s' % (self.device_name, midi_note))
            velocity = (velocity / 127.0) * (high - low) + low
        except Exception:
            if default is None:
                velocity = high
            else:
                velocity = default

        return velocity
