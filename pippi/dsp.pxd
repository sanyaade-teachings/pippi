#cython: language_level=3

from pippi.soundbuffer cimport SoundBuffer
from pippi.wavesets cimport Waveset
from pippi.wavetables cimport Wavetable

cdef double _mag(double[:,:] snd)
cpdef double mag(SoundBuffer snd)
cpdef list scale(list source, double fromlow=*, double fromhigh=*, double tolow=*, double tohigh=*, bint log=*)
cpdef double scalef(double source, double fromlow=*, double fromhigh=*, double tolow=*, double tohigh=*, bint log=*, bint exp=*)
cpdef list snap(list source, double mult=*, object pattern=*)
cpdef double tolog(double value, int base=*)
cpdef double toexp(double value, int base=*)
cpdef SoundBuffer mix(list sounds, align_end=*)
cpdef Wavetable randline(int numpoints, double lowvalue=*, double highvalue=*, int wtsize=*)
cpdef Wavetable wt(object values, object lowvalue=*, object highvalue=*, int wtsize=*)
cpdef Waveset ws(object values=*, object crossings=*, int offset=*, int limit=*, int modulo=*, int samplerate=*, list wavesets=*)
cpdef Wavetable win(object values, object lowvalue=*, object highvalue=*, int wtsize=*)
cpdef SoundBuffer stack(list sounds)
cpdef SoundBuffer buffer(object frames=*, double length=*, int channels=*, int samplerate=*)
cpdef SoundBuffer bufferfrom(SoundBuffer src)
cpdef SoundBuffer read(object filename, double length=*, int offset=*)
cpdef list readall(str path, double length=*, int offset=*)
cpdef SoundBuffer readchoice(str path)
cpdef double rand(double low=*, double high=*)
cpdef int randint(int low=*, int high=*)
cpdef object choice(list choices)
cpdef void seed(object value=*)
cpdef void randmethod(str method=*)
cpdef dict randdump()
cpdef SoundBuffer render(list events, object callback, int channels=*, int samplerate=*)
