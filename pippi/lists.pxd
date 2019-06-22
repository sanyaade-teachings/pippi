#cython: language_level=3

cdef double[:] _scale(double[:] out, double[:] source, double fromlow, double fromhigh, double tolow, double tohigh)
cpdef list scale(list source, double fromlow=*, double fromhigh=*, double tolow=*, double tohigh=*)
cdef double[:] _snap_pattern(double[:] out, double[:] source, double[:] pattern)
cdef double[:] _snap_mult(double[:] out, double[:] source, double mult)
cpdef list snap(list source, double mult=*, object pattern=*)
