#cython: language_level=3

cdef class Waveset:
    cdef public double[:] raw
    cdef public list sets
    cdef public int crossings
    cdef public int max_length
    cdef public int min_length

