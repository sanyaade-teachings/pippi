cdef double _linear_point(double[:] data, double phase) nogil
cpdef double linear_point(double[:] data, double pos)
cdef double _linear_pos(double[:] data, double pos) nogil
cdef double[:] _linear(double[:] data, int length)
cdef double _trunc_point(double[:] data, double pos) nogil
