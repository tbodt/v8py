#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "jsexception.h"

PyTypeObject js_exception_type = {
    PyObject_HEAD_INIT(NULL)
};
int js_exception_type_init() {
    js_exception_type.tp_name = "v8py.JSException";
    js_exception_type.tp_base = (PyTypeObject *) PyExc_Exception;
    js_exception_type.tp_basicsize = sizeof(js_exception);
    js_exception_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_exception_type.tp_doc = "";

    js_exception_type.tp_dealloc = (destructor) js_exception_dealloc;
    return PyType_Ready(&js_exception_type);
}

PyObject *js_exception_new(Local<Value> exception, Local<Message> message) {
    HandleScope hs(isolate);
    Local<Context> no_ctx;
    js_exception *self = (js_exception *) js_exception_type.tp_alloc(&js_exception_type, 0);
    PyErr_PROPAGATE(self);
    self->exception.Reset(isolate, exception);
    self->message.Reset(isolate, message);

    self->py_message = py_from_js(exception->ToString(), no_ctx);
    self->py_args = PyTuple_New(1);
    Py_INCREF(self->py_message);
    PyTuple_SetItem(self->py_args, 0, self->py_message);
    return (PyObject *) self;
}

void js_exception_dealloc(js_exception *self) {
    self->exception.Reset();
    self->message.Reset();
    self->ob_type->tp_free((PyObject *) self);
}
