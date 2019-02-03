#cython: language_level=3

from pippi cimport wavetables

cpdef list topositions(object p, double beat, double length, wavetables.Wavetable lfo=*)
cpdef list pattern(object pat, double bpm=*, double length=*, double swing=*, double div=*, object lfo=*, double delay=*)

