"""
Monotonic clock stuff adapted from Armin Ronacher's SO answer:
http://stackoverflow.com/a/1205762/5564

Thanks!
"""

import ctypes
import os
import dsp
import time
import utils

class timespec(ctypes.Structure):
    _fields_ = [
        ('tv_sec', ctypes.c_long),
        ('tv_nsec', ctypes.c_long)
    ]


try:
    librt = ctypes.CDLL('librt.so.1', use_errno=True)
    clock_gettime = librt.clock_gettime
    clock_gettime.argtypes = [ctypes.c_int, ctypes.POINTER(timespec)]

    def monotonic():
        t = timespec()
        if clock_gettime(4, ctypes.pointer(t)) != 0:
            errno_ = ctypes.get_errno()
            raise OSError(errno_, os.strerror(errno_))

        return t.tv_sec + t.tv_nsec * 1e-9

except OSError:
    utils.log('Monotonic clock disabled')
    def monotonic():
        return time.time()

def delay(length):
    """ Length in frames """
    
    duration = dsp.fts(length)
    start = monotonic()

    while monotonic() < start + duration:
        pass
