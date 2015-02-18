from pippi import dsp

def test_tone():
    out = dsp.tone(length=dsp.stf(1), freq=220, wavetype='sine2pi', amp=1, phase=0, offset=0)
    assert dsp.flen(out) == dsp.stf(1)
