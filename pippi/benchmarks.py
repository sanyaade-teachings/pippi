""" Run this with `python -m pippi.benchmarks` to see some 
    timing information for synthesis etc.
"""
import timeit

init = """\
from pippi import oscs
osc = oscs.Osc()
"""

basic = """\
from pippi import oscs
osc = oscs.Osc()
out = osc.play({length})
"""

pwsine = """\
from pippi import oscs, wavetables
wavetable = wavetables.wavetable('sine', {wtsize})
window = wavetables.window('sine', {wtsize})
pulsewidth = 0.95
osc = oscs.Osc(wavetable, window=window, pulsewidth=pulsewidth)
out = osc.play({length})
"""

pwsinemod = """\
from pippi import oscs, wavetables
wavetable = wavetables.wavetable('sine', {wtsize})
window = wavetables.window('sine', {wtsize})
mod = wavetables.window('sine', {wtsize})
pulsewidth = 0.95
osc = oscs.Osc(wavetable, window=window, mod=mod, pulsewidth=pulsewidth)
out = osc.play({length})
"""


if __name__ == '__main__':
    wtsize = 1024

    init_time = timeit.timeit(stmt=init, number=1000)
    print('init 1000x...')
    print(round(init_time, 2), '\n', init)

    run_times = [
        (1, 44100 * 10), 
        (10, 44100), 
        (100, 44100 * 2), 
    ]

    print('')
    print('basic')
    for nruns, length in run_times:
        print('%s %s second renders w/default settings...' % (nruns, round(length / 44100, 2)))
        basic_time = timeit.timeit(stmt=basic.format(length=44100), number=nruns)
        t = round(basic_time, 2)
        print('total time %s\n%s %s\n%s' % (t, round(t/nruns, 2), 'avg per run', basic.format(length=44100)))
        print('')

    print('')
    print('pulsewidth w/sine window')
    for nruns, length in run_times:
        print('%s %s second renders pulsewidth w/sine window...' % (nruns, round(length / 44100, 2)))
        pwsine_time = timeit.timeit(stmt=pwsine.format(length=44100, wtsize=wtsize), number=nruns)
        t = round(pwsine_time, 2)
        print('total time %s\n%s %s\n%s' % (t, round(t/nruns, 2), 'avg per run', pwsine.format(length=44100, wtsize=wtsize)))
        print('')

    print('')
    print('pulsewidth w/sine window & mod')
    for nruns, length in run_times:
        print('%s %s second renders pulsewidth w/sine window & mod...' % (nruns, round(length / 44100, 2)))
        pwsinemod_time = timeit.timeit(stmt=pwsinemod.format(length=44100, wtsize=wtsize), number=nruns)
        t = round(pwsinemod_time, 2)
        print('total time %s\n%s %s\n%s' % (t, round(t/nruns, 2), 'avg per run', pwsinemod.format(length=44100, wtsize=wtsize)))
        print('')
    print('')

