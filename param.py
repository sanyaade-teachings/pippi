import re

patterns = {
        'ms': re.compile(r'^ms\d+\.?\d*|\d+\.?\d*ms$'),         # Milliseconds
        's': re.compile(r'^s\d+\.?\d*|\d+\.?\d*s$'),            # Seconds
        'f': re.compile(r'^s\d+|\d+f$'),                        # Frames
        'b': re.compile(r'^b\d+\.?\d*|\d+\.?\d*b$'),            # Beats
        'n': re.compile(r'\d+\.?\d*'),                          # Int or float 
        }

def split(cmd):
    cmds = {}
    for c in [ c.split(':') for c in cmd.split() ]:
        cmds[c[0]] = c[1] if len(c) > 1 else True

    return cmds

def istype(value, flag):
    return patterns[flag].search(value)

def convert(value, type='float'):
    result = patterns['n'].search(value).group(0)

    if type == 'float':
        result = float(result)
    elif type == 'int' or type == 'integer':
        # Python throws a ValueError if we try to convert a float string 
        # to an integer directly, so convert to float first to be safe.
        result = int(float(result)) 

    return result
    
