from pippi import dsp, fx, graph

snd = dsp.read('examples/sounds/harpc2.wav')
snd = snd.cut(0.4, 0.5)
snd = fx.go(snd, 
                factor=200, 
                minclip=0.125, 
                maxclip=0.5, 
                density=0.65, 
                minlength=0.01, 
                maxlength=0.04
            )

snd.write('fxgo.wav')
graph.waveform(snd, 'bit-fxgo.png')
