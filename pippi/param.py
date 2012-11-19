import re
from pippi import dsp

class Param:
    patterns = {
            'millisecond':  re.compile(r'^ms\d+\.?\d*|\d+\.?\d*ms$'), 
            'second':       re.compile(r'^s\d+\.?\d*|\d+\.?\d*s$'), 
            'frame':        re.compile(r'^f\d+|\d+f$'), 
            'beat':         re.compile(r'^b\d+\.?\d*|\d+\.?\d*b$'), 
            'float':        re.compile(r'\d+\.?\d*'), 
            'integer':      re.compile(r'\d+'), 

            'note':         re.compile(r'[a-gA-G][#b]?'), 
            'alphanumeric':      re.compile(r'\w+'), 
            'integer-list':      re.compile(r'\d+(.\d+)*'), 
            'note-list':      re.compile(r'[a-gA-G][#b]?(.[a-gA-G][#b]?)*'), 
            'string-list':      re.compile(r'[a-zA-Z]+(.[a-zA-Z]+)*'), 
            }

    def __init__(self, data={}):
        self.data = { param: { 'value': value } for (param, value) in data.iteritems() }

    def istype(self, value, flag):
        return self.patterns[flag].search(value)

    def convert(self, value, input_type, output_type):
        # This is a dumb way to do this. Rehearsal approaches though...
        if input_type == 'string-list':
            return self.convert_string_list(value, output_type)

        if input_type == 'integer-list':
            return self.convert_integer_list(value, output_type)

        if input_type == 'integer': 
            return self.convert_integer(value, output_type)

        if input_type == 'float':
            return self.convert_float(value, output_type)

        if input_type == 'beat':
            return self.convert_beat(value, output_type)

        if input_type == 'second':
            return self.convert_second(value, output_type)

        if input_type == 'millisecond':
            return self.convert_millisecond(value, output_type)

        if input_type == 'note':
            return value

        if input_type == 'note-list':
            return self.convert_note_list(value, output_type)

        if input_type == 'alphanumeric':
            return value


    def convert_float(self, value, output_type='float'):
        value = self.patterns['float'].search(value).group(0)

        if output_type == 'float':
            value = float(value)
        elif output_type == 'integer' or output_type == 'frame':
            value = int(float(value)) 
        elif output_type == 'integer-list':
            value = [int(float(value))]

        return value


    def convert_integer(self, value, output_type='integer'):
        value = self.patterns['integer'].search(value).group(0)

        if output_type == 'float':
            value = float(value)
        elif output_type == 'integer' or output_type == 'frame':
            value = int(float(value)) 
        elif output_type == 'integer-list':
            value = [int(float(value))]

        return value

    def convert_beat(self, value, output_type):
        value = self.patterns['float'].search(value).group(0)
        if output_type == 'frame':
            value = dsp.bpm2ms(self.bpm) / float(value)
            value = dsp.mstf(value)

        return value 

    def convert_millisecond(self, value, output_type):
        if output_type == 'frame':
            value = self.convert_float(value)
            value = dsp.mstf(value)

        return value 

    def convert_second(self, value, output_type):
        if output_type == 'frame':
            value = self.convert_float(value)
            value = dsp.stf(value)

        return value 

    def convert_note_list(self, value, output_type):
        if output_type == 'note-list':
            value = value.split('.')
        
        return value

    def convert_string_list(self, value, output_type):
        if output_type == 'string-list':
            value = value.split('.')
            value = [str(v) for v in value]

        return value

    def convert_integer_list(self, value, output_type):
        if output_type == 'integer-list':
            value = value.split('.')
            value = [int(v) for v in value]

        return value

    def get(self, param_name):
        """ Get a param value by its key and 
            convert from an acceptable input type 
            to the specified output type.
            """
        # TODO: This whole thing has a stupid, convoluted flow. Fix.

        if param_name in self.data:
            param = self.data[param_name]
        else:
            return False

        if 'accepts' in param and 'type' in param:
            # Loop through each acceptable input type
            # and check to see if param value matches
            # one of them. 
            for input_type in param['accepts']:
                if self.istype(param['value'], input_type):
                    # At the first match, convert param value to target type
                    # and break out of the loop.
                    param = self.convert(param['value'], input_type, param['type'])
                    break
            else:
                # Return false when input doesn't match any of the accepted types
                param = False

        elif 'value' in param and param['value'] is True:
            param = param['value']

        return param 


def unpack(cmd, config):
    """ Format an input string as an expanded dictionary based on 
        required configuration options.
        """

    params = Param() 

    # Split space-separated params into a list
    cmds = cmd.strip().split()

    # Split colon-separated key:value param pairs
    # into lists.
    cmds = [ cmd.split(':') for cmd in cmds ]

    # Try to find generator from the list of
    # registered generators for the session
    for cmd in cmds:
        # Look up the list of registered generators and try to expand
        # the shortname into the full generator name
        if len(cmd) == 1 and cmd[0] in config['generators']:
            params.data['generator'] = config['generators'][cmd[0]]

        # Look up the list of registered params and try to expand
        # the shortname into the full param name, leaving the value 
        # untouched. 
        elif len(cmd) == 2 and cmd[0] in config['params']:
            param = config['params'][cmd[0]]
            param['value'] = cmd[1]
            params.data[param['name']] = param

        elif len(cmd) == 1 and cmd[0] not in config['generators']:
            if cmd[0] in config['params']:
                param = config['params'][cmd[0]]
                if 'name' in param:
                    cmd[0] = param['name']

            params.data[cmd[0]] = { 'name': cmd[0], 'value': True }

    return params

