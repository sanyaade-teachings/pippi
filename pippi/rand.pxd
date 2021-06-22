#cython: language_level=3

cdef extern from "pippicore.h":
    ctypedef struct lprand_t:
        pass

    extern const lprand_t LPRand


cpdef void seed(object value=*)
cpdef double rand(double low=*, double high=*)
cpdef int randint(int low=*, int high=*)
cpdef object choice(list choices)

