#cython: language_level=3


cpdef char make_trigger(list solenoid_number):
    cdef char msg, f
    cdef list sole_flags = [
        LPSOLEALL,
        LPSOLE1,
        LPSOLE2,
        LPSOLE3,
        LPSOLE4,
        LPSOLE5,
        LPSOLE6,
    ]

    msg = 0
    for f in solenoid_number:
        msg |= <char>f

    return msg
