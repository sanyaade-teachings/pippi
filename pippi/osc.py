import liblo
from pippi import dsp

def input_log(ns, active=True):
    if active:
        if not hasattr(ns, 'osc_log_active'):
            setattr(ns, 'osc_log_active', True)
    else:
        delattr(ns, 'osc_log_active')

def validate_address(address):
    """ TODO: add address validation """
    return False

class OscListener(liblo.ServerThread):
    def __init__(self, port, ns):
        liblo.ServerThread.__init__(self, port)
        self.ns = ns

    @liblo.make_method(None, None)
    def handler(self, path, args):
        if hasattr(self.ns, 'osc_log_active'):
            dsp.log('%s %s %s' % (self.port, path, args))

        setattr(self.ns, '%s%s' % (self.port, path), args)
        
class OscManager(object):
    def __init__(self, ns, port=None):
        self.ns = ns
        self.port = port

    def setPort(self, port):
        self.port = port

    def get(self, path, default=None, port=None):
        if port is None and self.port is not None:
            port = self.port
        
        try:
            args = getattr(self.ns, '%s%s' % (port, path))
        except AttributeError:
            args = default

        return args
