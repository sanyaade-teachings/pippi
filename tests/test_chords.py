from pippi import tune

def test_getQuality():
    assert tune.getQuality('ii') == '-'
    assert tune.getQuality('II') == '^'
    assert tune.getQuality('vi69') == '-'
    assert tune.getQuality('vi6/9') == '-'
    assert tune.getQuality('ii7') == '-'
    assert tune.getQuality('v*9') == '*'

def test_getExtension():
    assert tune.getExtension('ii') == ''
    assert tune.getExtension('II') == ''
    assert tune.getExtension('vi69') == '69'
    assert tune.getExtension('vi6/9') == '69'
    assert tune.getExtension('ii7') == '7'
    assert tune.getExtension('v*9') == '9'

def test_getIntervals():
    assert tune.getIntervals('ii') == ['P1', 'm3', 'P5']
    assert tune.getIntervals('II') == ['P1', 'M3', 'P5']
    assert tune.getIntervals('II7') == ['P1', 'M3', 'P5', 'm7']
    assert tune.getIntervals('v6/9') == ['P1', 'm3', 'P5', 'M6', 'M9']

def test_addIntervals():
    assert tune.addIntervals('P5','P8') == 'P12'
    assert tune.addIntervals('m3','P8') == 'm10'
    assert tune.addIntervals('m3','m3') == 'TT'
    assert tune.addIntervals('m3','M3') == 'P5'

def test_getChord():
    assert tune.chord('I7', key=440, ratios=tune.just) == [440.0, 550.0, 660.0, 792.0] 


def test_getRatiofromInterval():
    assert tune.getRatioFromInterval('P1', tune.just) == 1.0
    assert tune.getRatioFromInterval('P5', tune.just) == 1.5
    assert tune.getRatioFromInterval('P8', tune.just) == 2.0
    assert tune.getRatioFromInterval('m10', tune.just) == 2.4
    assert tune.getRatioFromInterval('P15', tune.just) == 4.0
