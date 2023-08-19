#!/usr/bin/env python

import logging
from logging.handlers import SysLogHandler
import platform
import threading
import time
import warnings

from rtmidi import MidiIn
from rtmidi.midiutil import open_midiinput
from cymidi_statuslog import setcc, setnote, trigger_notemap

NOTE_ON = 144
NOTE_OFF = 128
CONTROL_CHANGE = 176
MSGTYPE = 240

logger = logging.getLogger('astrid-midiseq')
if not logger.handlers:
    if platform.system() == 'Darwin':
        log_path = '/var/run/syslog'
    else:
        log_path = '/dev/log'

    logger.addHandler(SysLogHandler(address=log_path))
    logger.setLevel(logging.DEBUG)
    warnings.simplefilter('always')


def midi_update_callback(event, device_id=None):
    msg, elapsed = event
    mt = msg[0] & MSGTYPE # extract the message type

    if mt == NOTE_ON:
        setnote(msg[1], msg[2], device_id)
        trigger_notemap(msg[1], device_id)

    elif mt == NOTE_OFF:
        setnote(msg[1], 0, device_id)

    elif mt == CONTROL_CHANGE:
        setcc(msg[1], msg[2], device_id)

    logger.info('NOTE_ON %s NOTE_OFF %s CC %s' % (
        NOTE_ON,
        NOTE_OFF,
        CONTROL_CHANGE,
    ))

    logger.info('msg %s NOTE_ON %s NOTE_OFF %s CC %s' % (
        msg, 
        (mt == NOTE_ON),
        (mt == NOTE_OFF),
        (mt == CONTROL_CHANGE),
    ))

def midi_device_listener(device_id, stop_event):
    try:
        midiin, port_name = open_midiinput(device_id, client_name='astrid-midistatus-device%s' % device_id)
        midiin.set_callback(midi_update_callback, device_id)
        while not stop_event.is_set():
            time.sleep(0.2)

        midiin.cancel_callback()
        del midiin
        logger.info('MIDI listener %s is now shut down' % device_id)

    except Exception as e:
        logger.exception('midi_device_listener exception:', str(e))

if __name__ == '__main__':
    midiin = MidiIn()
    midi_device_names = midiin.get_ports()
    del midiin

    stop_event = threading.Event()
    midi_devices = {}

    for device_id, device_name in enumerate(midi_device_names):
        logger.info('Starting MIDI listener thread for device %s (%s)' % (device_id, device_name))
        listener_thread = threading.Thread(target=midi_device_listener, args=(device_id, stop_event), name='astrid-mididevice%s-listener-thread' % device_id)
        midi_devices[device_id] = listener_thread
        listener_thread.start()

    try:
        while True:
            time.sleep(0.2)

    except KeyboardInterrupt as e:
        logger.info('MIDI listeners shutting down...')
        stop_event.set()
        for _, listener_thread in midi_devices.items():
            listener_thread.join()

