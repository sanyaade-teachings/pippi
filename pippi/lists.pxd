#cython: language_level=3

cdef double[:] _scale(double[:] out, double[:] source, double fromlow, double fromhigh, double tolow, double tohigh)
cpdef list scale(list source, double fromlow=*, double fromhigh=*, double tolow=*, double tohigh=*)
