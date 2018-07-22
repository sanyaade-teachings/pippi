cdef public double[:] _linear(double[:] data, int length)
cdef public double[:] _linear_inner(double[:] data, double[:] out, int length) nogil
cpdef double[:] linear(data, int length)
cdef public double[:] _hermite(double[:] data, int length)
cdef double _linear_point(double[:] data, double pos) nogil
cdef double _trunc_point(double[:] data, double pos) nogil
