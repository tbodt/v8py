import json
from pprint import pprint

import _v8py
import gevent
from gevent import pywsgi
import gevent.lock
import gevent.queue
from geventwebsocket.handler import WebSocketHandler
from geventwebsocket.exceptions import WebSocketError

class DevtoolsDebugger(_v8py.Debugger):
    def __init__(self, context):
        super().__init__(context)
        self.connect_lock = gevent.lock.Semaphore(0)
        self.queue = gevent.queue.Queue()
        self.ws = None

    def __call__(self, environ, start_response):
        if environ['PATH_INFO'] == '/':
            if self.ws is not None:
                print('reconnect. well fuck you')
                return []

            self.ws = environ['wsgi.websocket']
            v8_greenlet = gevent.spawn(self.talk_to_v8)
            while True:
                try:
                    message = self.ws.receive()
                except WebSocketError:
                    break
                message = json.loads(message)
                if message.get('method') == 'Runtime.runIfWaitingForDebugger':
                    print('releasing lock')
                    self.connect_lock.release()
                self.queue.put(message)
            self.ws = None
            v8_greenlet.kill()

            return []

        print(environ['PATH_INFO'])
        return []

    def talk_to_v8(self):
        for message in self.queue:
            print('>', end=' ')
            pprint(message)
            self.send(message)

    def handle(self, message):
        print('<', end=' ')
        pprint(message)
        self.ws.send(json.dumps(message))

    # Since gevent is handling our event loops, we just have run_loop block
    # until quit_loop is called. This also conveniently deals with nested
    # loops.
    def run_loop(self):
        print('run loop')
        self.talk_to_v8()
        # self.lock.acquire()
        # print('finishing loop')
    def quit_loop(self):
        print('killing loop')
        # self.lock.release()
        self.queue.put(StopIteration)

    def wait_for_connect(self):
        self.connect_lock.wait()

def start_devtools(context, port):
    debugger = DevtoolsDebugger(context)
    server = pywsgi.WSGIServer(('localhost', port), debugger, handler_class=WebSocketHandler)
    server.start()
    debugger.wait_for_connect()
    print('done waiting')
