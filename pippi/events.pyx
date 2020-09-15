#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer

cdef class Event:
    def __cinit__(self, 
            double onset=0,
            double length=1,
            double freq=220.0,
            double amp=1,
            double pos=0, 
            int count=0,
            object before=None,
            dict params=None,
            **kwargs,
        ):
        self.onset = onset
        self.length = length
        self.freq = freq
        self.amp = amp
        self.pos = pos
        self.count = count
        self._before = before

        if params is None:
            params = {}

        params.update(kwargs)

        self._params = params

    def __getattr__(self, key):
        return self.get(key)

    def get(self, key, default=None):
        if self._params is None:
            return default
        return self._params.get(key, default)

cdef SoundBuffer render(list events, object callback):
    cdef double end = events[-1].onset + events[-1].length
    cdef SoundBuffer out = SoundBuffer(length=end)

    cdef int count = 0
    cdef double pos = 0
    cdef Event event
    cdef SoundBuffer segment

    for event in events:
        event.count = count
        event.pos = (event.onset or 0) / end

        if event._before is not None:
            event.before = event._before(event)

        segment = callback(event)

        out.dub(segment, event.onset or 0)
        count += 1

    return out
