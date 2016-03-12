class ParamManager:
    def __init__(self, ns):
        self.ns = ns
        self.namespace = 'global'

    def setNamespace(self, namespace):
        self.namespace = namespace

    def set(self, param, value, namespace=None, throttle=None):
        if namespace is None:
            namespace = self.namespace

        if throttle is not None:
            last_updated = self.get('%s-last_updated' % param, time.time(), namespace='meta')

            if time.time() - last_updated >= throttle:
                self.set('%s-last_updated' % param, time.time(), namespace='meta')
                self.set(param, value, namespace)

        else:
            params = self.getAll(namespace)
            params[param] = value
            setattr(self.ns, namespace, params)

    def get(self, param, default=None, namespace=None, throttle=None):
        if namespace is None:
            namespace = self.namespace

        if throttle is not None:
            last_updated = self.get('%s-last_updated' % param, time.time(), namespace='meta')

            if time.time() - last_updated >= throttle:
                self.set('%s-last_updated' % param, time.time(), namespace='meta')
                self.set(param, default, namespace)

            return self.get(name, default, namespace)

        params = self.getAll(namespace)

        value = params.get(param, None)

        if value is None:
            value = default
            self.set(param, value, namespace)

        return value

    def getAll(self, namespace=None):
        if namespace is None:
            namespace = self.namespace

        try:
            params = getattr(self.ns, namespace)
        except AttributeError:
            params = {}

        return params


