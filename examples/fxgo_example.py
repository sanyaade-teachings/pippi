from pippi import dsp, fx, graph
import os

PATH = os.path.dirname(os.path.realpath(__file__))
print(__file__)

snd = dsp.read('%s/sounds/harpc2.wav' % PATH)
snd = fx.go(snd, 
                factor=200, 
                minclip=0.125, 
                maxclip=0.5, 
                density=0.65, 
                minlength=0.01, 
                maxlength=0.04
            )

snd.write('%s/fxgo.wav' % PATH)
graph.waveform(snd, '%s/fxgo.png' % PATH)
