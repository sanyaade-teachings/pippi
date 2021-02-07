from unittest import TestCase
from pippi import dsp, soundfont, fx, tune

dsp.seed()

class TestSoundfont(TestCase):
    def test_play(self):
        length = 1
        freq = 440.0
        amp = 0.99
        voice = 0
        out = soundfont.play("tests/sounds/florestan-gm.sf2", length, freq, amp, voice)
        out.write('tests/renders/soundfont_play.wav')

    def test_playall(self):
        events = []
        pos = 0
        length = 3
        freqs = tune.degrees(list(range(1,13)))

        while pos < length:
            events += [dsp.event(
                onset=pos, 
                length=dsp.rand(0.1, 3), 
                freq=dsp.choice(freqs)*2**dsp.randint(0,3), 
                amp=dsp.rand(), 
                voice=dsp.randint(0, 127),
            )]

            pos += 0.25
 
        out = soundfont.playall("tests/sounds/florestan-gm.sf2", events)
        out = fx.norm(out, 0.5)
        out.write('tests/renders/soundfont_playall.wav')


