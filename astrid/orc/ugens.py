"""
Mockup of a possible stream-based interface for astrid.

This might be best suited to live coding with nim to reduce 
overhead on the more traditional unit generator setup.

"""

def my_ugen(ctx):
    sample = ctx.input_sample
    sample *= 0.5 # do DSP stuff to the sample
    return sample

def mul1(ctx):
    return (ctx.input_sample + 1) * 0.5

def stream(ctx):
    ctx.graph.add('sine1', 'SineOsc', freq=330., amp='lfo1')
    ctx.graph.add('lfo1', 'SineOsc', freq=0.1, output='mul1') # Attach ugens to the output without adding to the graph
    ctx.graph.add('myugen', my_ugen, foo='bar')

    # Pipe the output of sine1 to the input of myugen.
    # Pipe the output of myugen to the DAC.
    ctx.graph.route('sine1 = myugen = dac')

    # Multiply the output of the adc and the sine1 ugen, 
    # and send it to the input of myugen. Pipe the output 
    # of myugen to the DAC.
    ctx.graph.route('adc * sine1 = myugen = dac')
