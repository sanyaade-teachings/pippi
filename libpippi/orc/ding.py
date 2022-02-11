from pippi import dsp, oscs

print("Lo! I have lo...aded")

def play(ctx):
    ctx.log("Rendering 1 second sine with env...")
    out = oscs.SineOsc(freq=330).play(1).env('pluckout')
    ctx.log("SineOsc: %s" % out.dur)
    yield out
