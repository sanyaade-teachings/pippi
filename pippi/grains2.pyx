from pippi.soundbuffer cimport SoundBuffer


cdef class Cloud2:
    def __cinit__(self, 
            SoundBuffer snd, 
            object window=None, 
            object position=None,
            object amp=1.0,
            object speed=1.0, 
            object spread=0.0, 
            object jitter=0.0, 
            object grainlength=0.2, 
            object grid=None,
            object mask=None,
            unsigned int wtsize=4096,
        ):

        self.channels = snd.channels
        self.samplerate = snd.samplerate

        """
        if window is None:
            window = 'sine'
        self.window = to_window(window)

        if position is None:
            self.position = Wavetable('phasor', 0, 1, window=True).data
        else:
            self.position = to_window(position)

        self.amp = to_window(amp)
        self.speed = to_window(speed)
        self.spread = to_window(spread)
        self.jitter = to_wavetable(jitter)
        self.grainlength = to_window(grainlength)

        if grid is None:
            self.grid = np.multiply(self.grainlength, 0.3)
        else:
            self.grid = to_window(grid)

        if mask is None:
            self.has_mask = False
        else:
            self.has_mask = True
            self.mask = array.array('i', mask)
        """

        self.snd = snd.frames

    def play(self, double length):
        cdef size_t i, c, sndlength, framelength, numgrains, maxgrainlength, mingrainlength
        cdef lpbuffer_t * snd
        cdef SoundBuffer out
        cdef lpformation_t * formation

        sndlength = <size_t>len(self.snd)
        framelength = <size_t>(length * self.samplerate)
        numgrains = 20
        maxgrainlength = 4800
        mingrainlength = 48

        snd = LPBuffer.create(sndlength, self.channels, self.samplerate)
        out = SoundBuffer(length=length, channels=self.channels, samplerate=self.samplerate)

        print(framelength, len(out.frames))

        for i in range(sndlength):
            for c in range(self.channels):
                snd.data[i * self.channels + c] = self.snd[i, c]

        formation = LPFormation.create(
                numgrains, 
                maxgrainlength, 
                mingrainlength, 
                sndlength, 
                self.channels, 
                self.samplerate
        )

        LPRingBuffer.write(formation.rb, snd)

        for i in range(framelength):
            LPFormation.process(formation)
            for c in range(self.channels):
                out.frames[i, c] = formation.current_frame.data[c]

        LPBuffer.destroy(snd)
        LPFormation.destroy(formation)

        return out


