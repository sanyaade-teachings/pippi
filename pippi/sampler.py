import numbers
from . import soundbuffer
from . import tune

class Sampler:
    def __init__(self, filename, pitch=None):
        self.pitch = tune.ntf(pitch or 'a4')
        self.sound = soundbuffer.SoundBuffer(filename=filename)

    def play(self, freq=440):
        speed = freq / self.pitch
        sound = self.sound.speed(speed)
        return sound

    def init_sampledb(self):
        self.db = sqlite3.connect(':memory:')
        self.db.cursor.execute('create table samples (freq real, sample binary);')

