#cython: language_level=3

import numpy as np
cimport numpy as np

cdef double[:] _scale(double[:] out, double[:] source, double fromlow, double fromhigh, double tolow, double tohigh):
    cdef int i = 0
    cdef double _tolow = min(tolow, tohigh)
    cdef double _tohigh = max(tolow, tohigh)
    cdef double _fromlow = min(fromlow, fromhigh)
    cdef double _fromhigh = max(fromlow, fromhigh)

    cdef double todiff = _tohigh - _tolow
    cdef double fromdiff = _fromhigh - _fromlow

    for i in range(len(source)):
        out[i] = ((source[i] - _fromlow) / fromdiff) * todiff + tolow

    return out

cpdef list scale(list source, double fromlow=-1, double fromhigh=1, double tolow=0, double tohigh=1):
    cdef unsigned int length = len(source)
    cdef double[:] out = np.zeros(length, dtype='d')
    cdef double[:] _source = np.array(source, dtype='d')
    return np.array(_scale(out, _source, fromlow, fromhigh, tolow, tohigh), dtype='d').tolist()
