#cython: language_level=3

cdef double _mag(double[:,:] snd)
cpdef double rand(double low=*, double high=*)
cpdef void seed(object value=*)
cpdef int randint(int low=*, int high=*)
cpdef object choice(list choices)

