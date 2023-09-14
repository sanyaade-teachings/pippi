#!/usr/bin/env python

import ctypes
import heapq
import os
import logging
from logging.handlers import SysLogHandler
import platform
import threading
from threading import Thread, Lock
import time
from typing import NamedTuple
import warnings

import rtmidi
from rtmidi.midiutil import open_midioutput
from rtmidi.midiconstants import NOTE_ON, NOTE_OFF, CONTROL_CHANGE, PROGRAM_CHANGE, BANK_SELECT

MIDIEVENTQ_PATH = '/tmp/astrid-miditriggerq'
MIDIDEVICE_PATH = '/tmp/astrid-default-midiout-device'

logger = logging.getLogger('astrid-midiseq')
if not logger.handlers:
    if platform.system() == 'Darwin':
        log_path = '/var/run/syslog'
    else:
        log_path = '/dev/log'

    logger.addHandler(SysLogHandler(address=log_path))
    logger.setLevel(logging.DEBUG)
    warnings.simplefilter('always')

class MIDIEvent(ctypes.Structure):
    _fields_ = [
        ('onset', ctypes.c_double),
        ('now', ctypes.c_double),
        ('length', ctypes.c_double),
        ('type', ctypes.c_ubyte),
        ('note', ctypes.c_ubyte),
        ('velocity', ctypes.c_ubyte),
        ('program', ctypes.c_ubyte),
        ('bank', ctypes.c_ubyte),
        ('channel', ctypes.c_ubyte),
    ]

class ScheduledEvent:
    def __init__(self, timestamp, event):
        self.timestamp = timestamp
        self.event = event

    def __lt__(self, other):
        return self.timestamp < other.timestamp

class MIDIEventPriorityQueue:
    def __init__(self):
        self._queue = []
        self._lock = Lock()

    def put(self, message: MIDIEvent):
        with self._lock:
            heapq.heappush(self._queue, message)

    def get(self):
        with self._lock:
            return heapq.heappop(self._queue)

    def peek(self):
        with self._lock:
            return self._queue[0] if self._queue else None


def relay_thread_comrade(q, stop_event):
    logger.info('Starting relay thread')
    if not os.path.exists(MIDIDEVICE_PATH):
        port = 0
    else:
        with open(MIDIDEVICE_PATH, 'r') as f:
            port = int(f.read())

    logger.info('MIDI scheduler: Opening port %s' % port)
    out, port_name = open_midioutput(port, client_name="astrid-midiseq")

    with out:
        lastonset = 0
        logger.info('MIDI scheduler: Waiting for messages...')
        while True:
            if stop_event.is_set():
                logger.info('Stopping midi out relay thread')
                break

            while q.peek() is None or q.peek().timestamp > time.clock_gettime(time.CLOCK_MONOTONIC_RAW):
                time.sleep(0.001)

            se = q.get()
            e = se.event

            try:
                #out.send_message([CONTROL_CHANGE | e.channel, BANK_SELECT_MSB, e.bank_msb])
                #out.send_message([CONTROL_CHANGE | e.channel, BANK_SELECT_LSB, e.bank_lsb])
                #out.send_message([PROGRAM_CHANGE | e.channel, max(0, e.program-1)])
                msg = [e.type | e.channel, e.note, e.velocity]
                logger.debug('sending midi message on device %s: %s' % (port, msg))
                out.send_message(msg)
            except Exception as e:
                logger.error('Could not send midi out message %s' % str(e))
                break

            logger.info('MIDI scheduler: sent message %s' % str(e))
    del out

def event_thread_comrade(pq, stop_event):
    logger.info('MIDI Event relay: starting')
    if not os.path.exists(MIDIEVENTQ_PATH):
        os.mkfifo(MIDIEVENTQ_PATH)

    logger.info('MIDI Event relay: opening q')
    qfd = os.open(MIDIEVENTQ_PATH, os.O_RDWR)

    while True:
        if stop_event.is_set():
            break

        try:
            event_bytes = os.read(qfd, ctypes.sizeof(MIDIEvent))
            #assert(len(event_bytes) == ctypes.sizeof(MIDIEvent))
            event = MIDIEvent.from_buffer_copy(event_bytes)

            logger.info('Got event on midiq %s' % event)
            #now = time.clock_gettime(time.CLOCK_MONOTONIC_RAW)
            logger.info('Sending MIDI event to pq %s' % str(event))
            pq.put(ScheduledEvent(event.now + event.onset, event))
        except Exception as e:
            logger.error('Error reading from midiq: %s' % e)

        time.sleep(0.001)

    os.close(qfd)
    os.unlink(MIDIEVENTQ_PATH)

if __name__ == '__main__':
    pq = MIDIEventPriorityQueue()
    stop_event = threading.Event()
    event_thread = threading.Thread(target=event_thread_comrade, args=(pq, stop_event), name='astrid-midiseq-events')
    relay_thread = threading.Thread(target=relay_thread_comrade, args=(pq, stop_event), name='astrid-midiseq-relay')

    try:
        event_thread.start()
        relay_thread.start()

    except KeyboardInterrupt as e:
        stop_event.set()
        event_thread.join()
        relay_thread.join()

