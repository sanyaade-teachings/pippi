#cython: language_level=3

cdef extern from "pippicore.h":
    ctypedef double lpfloat_t

    ctypedef struct lprand_t:
        lpfloat_t logistic_seed
        lpfloat_t logistic_x

        lpfloat_t lorenz_timestep
        lpfloat_t lorenz_x
        lpfloat_t lorenz_y
        lpfloat_t lorenz_z
        lpfloat_t lorenz_a
        lpfloat_t lorenz_b
        lpfloat_t lorenz_c

        void (*seed)(int)

        lpfloat_t (*stdlib)(lpfloat_t, lpfloat_t)
        lpfloat_t (*logistic)(lpfloat_t, lpfloat_t)

        lpfloat_t (*lorenz)(lpfloat_t, lpfloat_t)
        lpfloat_t (*lorenzX)(lpfloat_t, lpfloat_t)
        lpfloat_t (*lorenzY)(lpfloat_t, lpfloat_t)
        lpfloat_t (*lorenzZ)(lpfloat_t, lpfloat_t)

        lpfloat_t (*rand_base)(lpfloat_t, lpfloat_t)
        lpfloat_t (*rand)(lpfloat_t, lpfloat_t)
        int (*randint)(int, int)
        int (*randbool)()
        int (*choice)(int)

    extern lprand_t LPRand


cpdef void seed(object value=*)
cpdef double rand(double low=*, double high=*)
cpdef int randint(int low=*, int high=*)
cpdef object choice(list choices)
cpdef void randmethod(str method=*)
cpdef dict randdump()

