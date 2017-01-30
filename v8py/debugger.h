#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <Python.h>
#include <v8.h>
#include <v8-inspector.h>
#include <memory>

#include "v8py.h"

using namespace v8;
using namespace v8_inspector;

class V8PyInspectorClient;
class V8PyChannel;

typedef struct _debugger {
    PyObject_HEAD
    Persistent<Context> context;
    V8PyInspectorClient *client;
    V8PyChannel *channel;
    std::unique_ptr<V8Inspector> inspector;
    std::unique_ptr<V8InspectorSession> session;
} debugger_c;
extern PyTypeObject debugger_type;
int debugger_type_init();

PyObject *debugger_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int debugger_init(debugger_c *self, PyObject *args, PyObject *kwargs);
void debugger_dealloc(debugger_c *self);
PyObject *debugger_send(debugger_c *self, PyObject *message);
PyObject *debugger_handle(debugger_c *self, PyObject *message);

class V8PyInspectorClient : public V8InspectorClient {
    public:
        V8PyInspectorClient(debugger_c *debugger) : debugger_(debugger) {}
        Local<Context> ensureDefaultContextInGroup(int context_group_id) override {
            printf("returning context\n");
            return debugger_->context.Get(isolate);
        }

        void runMessageLoopOnPause(int context_group_id) override;
        void quitMessageLoopOnPause() override;

    private:
        debugger_c *debugger_;
};

class V8PyChannel : public V8Inspector::Channel {
    public:
        V8PyChannel(debugger_c *debugger) : debugger_(debugger) {}
    private:
        void sendResponse(int callId, std::unique_ptr<StringBuffer> message) override {
            handle_message(message);
        }
        void sendNotification(std::unique_ptr<StringBuffer> message) override {
            handle_message(message);
        }
        void flushProtocolNotifications() override {}

        void handle_message(const std::unique_ptr<StringBuffer> &message);

        debugger_c *debugger_;
};

#endif
