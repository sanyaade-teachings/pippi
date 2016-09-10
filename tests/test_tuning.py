from pippi import tune

def test_ntf():
    assert tune.ntf('a4') == 440

def test_fromdegrees():
    assert tune.fromdegrees([1], octave=4, root='a') == [440]
    assert tune.fromdegrees([1,8], octave=4, root='a') == [440,880]
    assert tune.fromdegrees([1,8], octave=3, root='a') == [220,440]
