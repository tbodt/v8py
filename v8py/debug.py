import _v8py

class Debugger(_v8py.Debugger):
    def __init__(self, context):
        super(self.__class__, self).__init__(context)
        self.sequence = 0
        self.last_message = None
        self.loop_nesting = 0

    def send(self, method, **kwargs):
        message = {
            'id': self.sequence,
            'method': method,
            'params': kwargs,
        }
        self.sequence += 1
        super(self.__class__, self).send(message)
        assert self.last_message is not None
        assert self.last_message['id'] == self.sequence - 1
        if 'error' in self.last_message:
            raise DebuggerError(self.last_message['error']['message'])
        return self.last_message['result']

    def handle(self, message):
        print(message)
        if 'params' in message:
            # it's an event, not a response
            print('event', message['method'], message['params'])
        else:
            self.last_message = message

    def run_loop(self):
        self.loop_nesting += 1
        nesting = self.loop_nesting
        while self.loop_nesting >= nesting:
            self.loop()

    def quit_loop(self):
        self.loop_nesting -= 1

    def loop(self):
        import pdb;pdb.set_trace()

class DebuggerError(Exception): pass
