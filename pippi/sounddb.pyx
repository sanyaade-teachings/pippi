#cython: language_level=3

import contextlib
import os
from pathlib import Path
import sqlite3

import numpy as np
cimport numpy as np

from pippi import midi
from pippi.soundbuffer cimport SoundBuffer
from pippi cimport mir

np.import_array()

sqlite3.register_adapter(np.ndarray, lambda a: a.tobytes())
sqlite3.register_converter('BUFFER', lambda b: np.frombuffer(b))


cdef class SoundDB:
    def __cinit__(SoundDB self, object snd=None, object filename=None, object offset=None, str dbname=None, str dbpath=None, bint overwrite=False):
        if dbpath is None:
            dbpath = '.'

        if dbname is None:
            dbname = 'sound'

        fullpath = Path(dbpath) / dbname

        if fullpath.exists() and overwrite:
            with contextlib.suppress(FileNotFoundError):
                os.remove(fullpath)
        
        init = False
        if not fullpath.exists():
            init = True

        self.path = str(fullpath)

        self.db = sqlite3.connect(self.path)
        self.db.row_factory = sqlite3.Row
        self.c = self.db.cursor()

        if init:
            self.setup()

        if isinstance(snd, list):
            f = filename or ''
            o = offset or 0
            for i, s in enumerate(snd):
                if isinstance(filename, list):
                    f = filename[i]
                if isinstance(offset, list):
                    o = offset[i]
                self.ingest(s, f, o)

        elif isinstance(snd, SoundBuffer):
            self.ingest(snd, filename or '', offset or 0)

    def setup(SoundDB self):
        sql = """CREATE TABLE sounds (
            id INTEGER PRIMARY KEY, 
            filename TEXT, 
            offset INTEGER,
            channels INTEGER, 
            samplerate INTEGER,
            duration REAL, 
            magnitude REAL, 

            bandwidth BUFFER, bandwidth_min REAL, bandwidth_max REAL, bandwidth_avg REAL, 
            flatness BUFFER, flatness_min REAL, flatness_max REAL, flatness_avg REAL, 
            rolloff BUFFER, rolloff_min REAL, rolloff_max REAL, rolloff_avg REAL, 
            centroid BUFFER, centroid_min REAL, centroid_max REAL, centroid_avg REAL, 
            contrast BUFFER, contrast_min REAL, contrast_max REAL, contrast_avg REAL)
        """
        self.c.execute(sql)
        self.db.commit()

        sql = """CREATE TABLE events (
            id INTEGER PRIMARY KEY, 
            voice_id INTEGER,
            group_id INTEGER,
            take_id INTEGER,
            start REAL,
            now REAL,
            onset REAL, 
            freq REAL,
            amp REAL,
            midi_msg_type INTEGER,
            channel INTEGER,
            note INTEGER,
            velocity INTEGER
        )
        """
        self.c.execute(sql)
        self.db.commit()

    def get_midi_events(SoundDB self, double start, double end, int voice_id=-1, int group_id=-1, int take_id=-1, int channel=0):
        sql = "SELECT onset, note, velocity, channel from events where voice_id=? and group_id=? and channel=? and onset < ? and onset > ? %sorder by onset"
        if take_id > 0:
            r = self.c.execute(sql % "and take_id=? ", (voice_id, group_id, channel, end, start, take_id))
        else:
            r = self.c.execute(sql % "", (voice_id, group_id, end, start))

        return r.fetchall()

    def add_midi_event(SoundDB self, int midi_msg_type, double start, double now, int note, int velocity, int voice_id=-1, int group_id=-1, int take_id=0, int channel=0):
        self.c.execute("INSERT INTO events VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)", (
            None, voice_id, group_id, take_id,
            start, now, now-start, 
            midi.mtof(note), velocity/127, 
            midi_msg_type, channel, note, velocity
        ))
        self.db.commit()

    def ingest(SoundDB self, SoundBuffer snd, str filename=None, int offset=0):
        cdef np.ndarray vector = mir.flatten(snd)

        cdef np.ndarray bandwidth = mir._bandwidth(vector, snd.samplerate, mir.DEFAULT_WINSIZE)
        cdef double bandwidth_min = np.min(bandwidth)
        cdef double bandwidth_max = np.max(bandwidth)
        cdef double bandwidth_avg = np.average(bandwidth)

        cdef np.ndarray flatness = mir._flatness(vector, mir.DEFAULT_WINSIZE)
        cdef double flatness_min = np.min(flatness)
        cdef double flatness_max = np.max(flatness)
        cdef double flatness_avg = np.average(flatness)

        cdef np.ndarray rolloff = mir._rolloff(vector, snd.samplerate, mir.DEFAULT_WINSIZE)
        cdef double rolloff_min = np.min(rolloff)
        cdef double rolloff_max = np.max(rolloff)
        cdef double rolloff_avg = np.average(rolloff)

        cdef np.ndarray centroid = mir._centroid(vector, snd.samplerate, mir.DEFAULT_WINSIZE)
        cdef double centroid_min = np.min(centroid)
        cdef double centroid_max = np.max(centroid)
        cdef double centroid_avg = np.average(centroid)

        cdef np.ndarray contrast = mir._contrast(vector, snd.samplerate, mir.DEFAULT_WINSIZE)
        cdef double contrast_min = np.min(contrast)
        cdef double contrast_max = np.max(contrast)
        cdef double contrast_avg = np.average(contrast)

        self.c.execute("INSERT INTO sounds VALUES (%s)" % ', '.join(['?']*27), (
            None, filename, offset, snd.channels, snd.samplerate, snd.dur, snd.mag,
            bandwidth, bandwidth_min, bandwidth_max, bandwidth_avg,
            flatness, flatness_min, flatness_max, flatness_avg,
            rolloff, rolloff_min, rolloff_max, rolloff_avg,
            centroid, centroid_min, centroid_max, centroid_avg,
            contrast, contrast_min, contrast_max, contrast_avg,
        ))

        self.db.commit()

    def query(self, sql):
        r = self.c.execute(sql)
        return r.fetchone()

    def queryall(self, sql):
        r = self.c.execute(sql)
        return r.fetchall()

    def __del__(SoundDB self):
        self.db.close()



