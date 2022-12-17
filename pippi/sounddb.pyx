#cython: language_level=3

import contextlib
import os
from pathlib import Path
import sqlite3

import numpy as np
cimport numpy as np

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



