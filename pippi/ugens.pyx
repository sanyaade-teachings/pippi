from collections import defaultdict
import warnings

cimport numpy as np
import numpy as np

from pippi.defaults cimport DEFAULT_SAMPLERATE, DEFAULT_CHANNELS
from pippi.soundbuffer cimport SoundBuffer

np.import_array()

cdef dict UGEN_INPUTNAME_MAP = {
    'sine.freq': USINEIN_FREQ,
    'sine.phase': USINEIN_PHASE,
}

cdef dict UGEN_OUTPUTNAME_MAP = {
    'sine.output': USINEOUT_MAIN,
    'sine.freq': USINEOUT_FREQ,
    'sine.phase': USINEOUT_PHASE,
}


cdef class Node:
    def __cinit__(self, str name, str ugen, *args, **kwargs):
        if ugen == 'sine':
            self.u = create_sine_ugen()
        else:
            raise AttributeError('Invalid ugen type "%s"' % ugen)

        self.ugen_name = ugen
        self.name = name
        self.connections = defaultdict(list)
        for k, v in kwargs.items():
            self.set_param(k, v)

    def __dealloc__(self):
        if self.u != NULL:
            self.u.destroy(self.u)

    def get_output(self, str name):
        return self.u.get_output(self.u, UGEN_OUTPUTNAME_MAP.get('.'.join([self.ugen_name, name]), 0))

    def set_param(self, str name, double value):
        self.u.set_param(self.u, UGEN_INPUTNAME_MAP.get('.'.join([self.ugen_name, name]), 0), value)

    def process(self):
        self.u.process(self.u)


cdef class Graph:
    def __cinit__(self):
        self.nodes = {}
        self.outputs = defaultdict(float)

    def add_node(self, str name, str ugen, *args, **kwargs):
        self.nodes[name] = Node(name, ugen, *args, **kwargs)

    def connect(self, str a, str b, double mult=1, double add=0):
        anodename, aportname = tuple(a.split('.'))
        bnodename, bportname = tuple(b.split('.'))
        self.nodes[anodename].connections[aportname] += [(bnodename, bportname, mult, add)]

    def render(self, double length, int samplerate=DEFAULT_SAMPLERATE, int channels=DEFAULT_CHANNELS):
        cdef size_t framelength = <size_t>(length * samplerate)
        cdef double sample = 0
        cdef size_t i
        cdef int c

        cdef double[:,:] out = np.zeros((framelength, channels))

        for i in range(framelength):
            sample = 0

            # first process all the nodes
            for _, node in self.nodes.items():
                node.process()

                # connect the outputs to the inputs
                for portname, connections in node.connections.items():
                    port = node.get_output(portname)
                    for connode, conport, mult, add in connections:
                        value = port * mult + add
                        if connode == 'main' and conport == 'output':
                            sample += value
                        else:
                            self.nodes[connode].set_param(conport, value)

            for c in range(channels):
                out[i,c] = sample

        return SoundBuffer(out, samplerate=samplerate, channels=channels)

