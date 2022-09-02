#cython: language_level=3


cdef extern from "solenoids.h":
    int LPSOLENUM
    int TICKS_UNTIL_RELEASE

    enum LPSOLE_SELECTION_FLAGS:
        LPSOLEALL,
        LPSOLE1,
        LPSOLE2, 
        LPSOLE3,
        LPSOLE4, 
        LPSOLE5, 
        LPSOLE6, 

    enum LPSOLE_STATE_FLAGS:
        LPSOLE_STATE_TRIGGERED,
        LPSOLE_STATE_LFO_ENABLED,
        LPSOLE_STATE_RND_ENABLED

    ctypedef struct lpsole_t:
        char flags
        unsigned long trigger_start
        unsigned long lfo_start
        float lfo_period

cpdef char make_trigger(list solenoid_number)
