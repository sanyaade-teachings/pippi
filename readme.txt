Very alpha!

Performance is best when rt.pyx and dsp.pyx are compiled as cython modules, otherwise just rename the files to add a .py extension.

Basic usage:

    python console.py

    Pippi Console
    pippi: dr o:2 wf:impulse n:d.a.e t:30 h:1.2.3

Starts the pippi console and generates a 30 second long stack of impulse trains on D, A, and E each with partials 1, 2, and 3.

Take a look at the scripts in orc/ for more arguments and generators.
