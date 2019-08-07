from unittest import TestCase
from pippi import dsp, soundfont, fx

class TestSoundfont(TestCase):
    def test_play(self):
        length = 3
        freq = 660
        amp = 1
        voice = 1
        out = soundfont.play("tests/sounds/florestan-gm.sf2", length, freq, amp, voice)
        out.write('tests/renders/soundfont_play.wav')

    def test_playall(self):
        events = [
            (0, 1, 330, 1, dsp.randint(0, 127)), 
            (0.3, 1.5, 440, 1, dsp.randint(0, 127)), 
            (0.6, 2, 550, 1, dsp.randint(0, 127)), 
            (0.9, 3, 660, 1, dsp.randint(0, 127)), 
            (1.2, 1.5, 770, 1, dsp.randint(0, 127)), 
            (1.5, 1, 880, 1, dsp.randint(0, 127)), 
        ]        
        out = soundfont.playall("tests/sounds/florestan-gm.sf2", events)
        out.write('tests/renders/soundfont_playall.wav')


