#cython: language_level=3

from libc.time cimport time
from libc.math cimport round

cpdef void seed(object value=None):
    if value is None:
        LPRand.seed(time(NULL))
    else:
        LPRand.seed(<unsigned int>value)

cpdef void randmethod(str method='normal'):
    if method == 'logistic':
        LPRand.rand_base = LPRand.logistic
    elif method == 'lorenz':
        LPRand.rand_base = LPRand.lorenz
    elif method == 'lorenzX':
        LPRand.rand_base = LPRand.lorenzX
    elif method == 'lorenzY':
        LPRand.rand_base = LPRand.lorenzY
    elif method == 'lorenzZ':
        LPRand.rand_base = LPRand.lorenzZ
    else:
        LPRand.rand_base = LPRand.stdlib

cpdef dict randdump():
    return dict(
        logistic_seed = LPRand.logistic_seed,
        logistic_x = LPRand.logistic_x,
        lorenz_timestep = LPRand.lorenz_timestep,
        lorenz_x = LPRand.lorenz_x,
        lorenz_y = LPRand.lorenz_y,
        lorenz_z = LPRand.lorenz_z,
        lorenz_a = LPRand.lorenz_a,
        lorenz_b = LPRand.lorenz_b,
        lorenz_c = LPRand.lorenz_c
    )

def randparams(domain=None, **kwargs):
    if domain is None:
        return None

    if domain == 'logistic':
        if 'seed' in kwargs:
            LPRand.logistic_seed = <double>kwargs['seed']

        if 'x' in kwargs:
            LPRand.logistic_x = <double>kwargs['x']

    if domain == 'lorenz':
        if 'timestep' in kwargs:
            LPRand.lorenz_timestep = <double>kwargs['timestep']

        if 'x' in kwargs:
            LPRand.lorenz_x = <double>kwargs['x']

        if 'y' in kwargs:
            LPRand.lorenz_y = <double>kwargs['y']

        if 'z' in kwargs:
            LPRand.lorenz_z = <double>kwargs['z']

        if 'a' in kwargs:
            LPRand.lorenz_a = <double>kwargs['a']

        if 'b' in kwargs:
            LPRand.lorenz_b = <double>kwargs['b']

        if 'c' in kwargs:
            LPRand.lorenz_c = <double>kwargs['c']

cpdef double rand(double low=0, double high=1):
    return LPRand.rand(low, high)

cpdef int randint(int low=0, int high=1):
    return LPRand.randint(low, high)

cpdef object choice(list choices):
    cdef int numchoices = <int>len(choices)
    if numchoices < 1: 
        return None
    cdef int choice_index = LPRand.choice(numchoices)
    return choices[choice_index]
