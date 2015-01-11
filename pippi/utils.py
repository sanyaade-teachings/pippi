import os
import struct
from datetime import datetime
from pippi import audio_params

def log(message, mode="a"):
    """ 
    Write to a temporary log file at ~/pippi.log for debugging.
    Set mode to "w" or similar to truncate logs when starting a new 
    session.
    """

    logfile = open(os.path.expanduser("~/pippi.log"), mode)
    logfile.write(str(message) + "\n")
    return logfile.close()

def flen(snd):
    """ 
    Returns the length of a sound in frames 

    ::

        >>> dsp.flen(dsp.tone(dsp.mstf(1))) == dsp.mstf(1)
        True

    """
    return len(snd) / (audio_params[1] + audio_params[0])

def byte_string(number):
    """ 
    Takes an integer (truncated to between -32768 and 32767) and returns a single 
    frame of sound. """
    number = cap(number, 32767, -32768)
    return struct.pack("<h", number)

def pack(number):
    """ Takes a float between -1.0 to 1.0 and returns a single frame of sound. 
    A wrapper for byte_string() """
    number = cap(number, 1.0, -1.0)
    number = (number + 1.0) * 0.5
    number = number * 65535 - 32768
    return byte_string(int(number))

def scale(low_target, high_target, low, high, pos):
    """ 
    Takes a target range (low, high) and source range (low, high) and 
    a value in the source range and returns a scaled value in the target range.
    
    To scale a value between 0 and 1 to a value between 0 and 100::

        >>> print dsp.scale(0, 100, 0, 1, 0.5)
        50.0

    """
    pos = float(pos - low) / float(high - low) 
    return pos * float(high_target - low_target) + low_target
    
def cap(num, max, min=0):
    """ 
    Takes a number and a maximum and minimum cap and returns a 
    truncated number within (inclusive) that range::

        >>> dsp.cap(500, 1, 0)
        1

        >>> dsp.cap(-42424242, 32767, -32768)
        -32768
    """

    if num < min:
        num = min
    elif num > max:
        num = max
    return num

def timestamp_filename():
    """ Convenience function that formats a datetime string for filenames::
        
            >>> dsp.timestamp_filename()
            '2015-10-21_07.28.00'
    """

    current_time = str(datetime.time(datetime.now()))
    current_time = current_time.split(':')
    current_seconds = current_time[2].split('.')
    current_time = current_time[0] + '.' + current_time[1] + '.' + current_seconds[0]
    current_date = str(datetime.date(datetime.now()))

    return current_date + "_" + current_time

