def linear(data, int length):
    out = []

    cdef int i = 0
    cdef int readindex = 0
    cdef int inputlength = len(data)
    cdef double phase = 0
    cdef double val, nextval, frac

    for i in range(length):
        readindex = int(phase) % inputlength

        val = data[readindex]

        try:
            nextval = data[readindex + 1]
        except IndexError:
            nextval = data[0]

        frac = phase - int(phase)

        val = (1.0 - frac) * val + frac * nextval

        out += [ val ]

        phase += inputlength * (1.0 / length)

    return out
