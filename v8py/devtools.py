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
        self.queues = []
        self.startup_lock = gevent.lock.Semaphore(0)
        self.ws = None

    def __call__(self, environ, start_response):
        if environ['PATH_INFO'] == '/':
            if self.ws is not None:
                print('reconnect. well fuck you')
                return []

            self.ws = environ['wsgi.websocket']
            gevent.spawn(self.run_loop)
            # wait for the queue to get added
            self.startup_lock.acquire()
            assert len(self.queues) == 1
            while True:
                try:
                    message = self.ws.receive()
                except WebSocketError:
                    break
                message = json.loads(message)
                if message.get('method') == 'Runtime.runIfWaitingForDebugger':
                    self.connect_lock.release()
                self.top_queue.put(message)
            self.ws = None
            v8_greenlet.kill()

            return []

        print(environ['PATH_INFO'])
        return []

    def handle(self, message):
        self.ws.send(json.dumps(message))

    def run_loop(self):
        queue = gevent.queue.Queue()
        self.queues.append(queue)
        # this has no effect if startup has finished, except to pointlessly
        # increment the counter. wow I'm so lazy I don't want to write a simple if statement
        self.startup_lock.release()
        for message in queue:
            if self.top_queue is not queue:
                self.top_queue.put(message)
            else:
                self.send(message)

    def quit_loop(self):
        self.queues.pop().put(StopIteration)

    @property
    def top_queue(self):
        return self.queues[-1]

    def wait_for_connect(self):
        self.connect_lock.wait()

def start_devtools(context, port):
    debugger = DevtoolsDebugger(context)
    server = pywsgi.WSGIServer(('localhost', port), debugger, handler_class=WebSocketHandler)
    server.start()
    print('started devtools server on port', port)
    print('open this url in chrome to continue')
    print('chrome-devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=localhost:{}'.format(port))
    debugger.wait_for_connect()
