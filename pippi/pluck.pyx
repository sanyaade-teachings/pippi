from pippi.soundbuffer cimport SoundBuffer
import numpy as np

cdef inline short DOUBLE_TO_SHORT(double x):
    return <int>(x*32768.0)

cdef class DelayLine:
    cdef short[:] buf
    cdef int length
    cdef int position
    cdef int end

    def __init__(self, int length):
        self.length = length
        self.buf = np.zeros(length, dtype='int16')
        self.position = 0
        self.end = length - 1

cdef class PluckedString:
    cdef DelayLine upper_rail
    cdef DelayLine lower_rail

    cdef double amp
    cdef double pitch
    cdef double pick
    cdef double pickup

    cdef short state
    cdef int pickup_location

    cdef int samplerate
    cdef int channels

    def __init__(self, double pitch=220.0, double pick=0.1, double pickup=0.2, double amp=1, int samplerate=44100, int channels=2):
        self.state = 0
        self.pitch = pitch
        self.pick = pick
        self.pickup = pickup
        self.amp = amp
        self.samplerate = samplerate
        self.channels = channels

        cdef int rail_length = <int>(<double>samplerate / pitch / 2.0 + 1.0)

        self.upper_rail = DelayLine(rail_length)
        self.lower_rail = DelayLine(rail_length)

        # Round pick position to nearest spatial sample.
        # A pick position at x = 0 is not allowed. 
        cdef int pickSample = <int>max(rail_length * pick, 1)
        cdef double upslope = amp/pickSample
        cdef double downslope = amp/(rail_length - pickSample - 1)
        cdef double[:] initial_shape = np.zeros(rail_length, dtype='d')

        for i in range(pickSample):
            initial_shape[i] = upslope * i

        for i in range(pickSample, rail_length):
            initial_shape[i] = downslope * (rail_length - 1 - i)

        # Initial conditions for the ideal plucked string.
        # "Past history" is measured backward from the end of the array.
        for i in range(self.lower_rail.length):
            self.lower_rail.buf[i] = DOUBLE_TO_SHORT(0.5 * initial_shape[i])

        for i in range(self.upper_rail.length):
            self.upper_rail.buf[i] = DOUBLE_TO_SHORT(0.5 * initial_shape[i])

        self.pickup_location = <int>(pickup * rail_length)

    cpdef short get_sample(PluckedString self, DelayLine dline, int position):
        return dline.buf[(dline.position + position) % dline.length]

    cpdef double next_sample(PluckedString self):
        cdef short yp0, ym0, ypM, ymM
        cdef short outsamp, outsamp1

        # Output at pickup location 
        # Returns spatial sample at location "position", where position zero
        # is equal to the current upper delay-line pointer position (x = 0).
        # In a right-going delay-line, position increases to the right, and
        # delay increases to the right => left = past and right = future.

        # Returns sample "position" samples into delay-line's past.
        # Position "0" points to the most recently inserted sample.
        outsamp = self.get_sample(self.upper_rail, self.pickup_location)

        # Returns spatial sample at location "position", where position zero
        # is equal to the current lower delay-line pointer position (x = 0).
        # In a left-going delay-line, position increases to the right, and
        # delay DEcreases to the right => left = future and right = past.
        outsamp1 = self.get_sample(self.lower_rail, self.pickup_location)

        outsamp += outsamp1

        ym0 = self.get_sample(self.lower_rail, 1) # Sample traveling into "bridge"
        ypM = self.get_sample(self.upper_rail, self.upper_rail.length - 2) # Sample to "nut"

        ymM = -ypM                    # Inverting reflection at rigid nut 

        # Implement a one-pole lowpass with feedback coefficient = 0.5 
        # outsamp = 0.5 * outsamp + 0.5 * insamp 
        self.state = (self.state >> 1) + (ym0 >> 1)

        yp0 = -self.state  # Reflection at yielding bridge 

        # String state update 

        # Decrements current upper delay-line pointer position (i.e.
        # the wave travels one sample to the right), moving it to the
        # "effective" x = 0 position for the next iteration.  The
        # "bridge-reflected" sample from lower delay-line is then placed
        # into this position.

        # Decrement pointer and then update 
        self.upper_rail.position -= 1
        self.upper_rail.buf[self.upper_rail.position % self.upper_rail.length] = yp0

        # Places "nut-reflected" sample from upper delay-line into
        # current lower delay-line pointer location (which represents
        # x = 0 position).  The pointer is then incremented (i.e. the
        # wave travels one sample to the left), turning the previous
        # position into an "effective" x = L position for the next
        # iteration.

        # Update and then increment pointer
        self.lower_rail.buf[self.lower_rail.position % self.lower_rail.length] = ymM
        self.lower_rail.position += 1

        return <double>(outsamp / 32768.0)


    def play(self, double length):
        cdef SoundBuffer out = SoundBuffer(length=length, channels=self.channels, samplerate=<int>self.samplerate)
        cdef int framelength = len(out)
        cdef double sample = 0

        for i in range(framelength):
            sample = self.next_sample()
            for c in range(self.channels):
                out.frames[i][c] = sample
        
        return out
