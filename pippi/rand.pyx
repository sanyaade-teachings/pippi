from libc.stdlib cimport rand as _rand
from libc.stdlib cimport RAND_MAX

cpdef double rand(double low=0, double high=1):
    return (_rand()/<double>RAND_MAX) * (high-low) + low

cpdef int randint(int low=0, int high=1):
    return <int>rand(low, high)

cpdef object choice(list choices):
    cdef int numchoices = <int>len(choices)
    cdef int choice_index = randint(0, numchoices-1)
    return choices[choice_index]
