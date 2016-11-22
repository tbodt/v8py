#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "pyclass.h"
#include "convert.h"
#include "exception.h"

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

void py_throw_js(Local<Value> js_exc, Local<Message> js_message) {
    if (js_exc->IsObject() && js_exc.As<Object>()->InternalFieldCount() == OBJECT_INTERNAL_FIELDS) {
        Local<Object> exc_object = js_exc.As<Object>();
        PyObject *exc_type = (PyObject *) exc_object->GetInternalField(2).As<External>()->Value();
        PyObject *exc_value = (PyObject *) exc_object->GetInternalField(1).As<External>()->Value();
        PyObject *exc_traceback = (PyObject *) exc_object->GetInternalField(3).As<External>()->Value();
        PyErr_Restore(exc_type, exc_value, exc_traceback);
    } else {
        PyObject *exception = js_exception_new(js_exc, js_message);
        if (exception == NULL) {
            return;
        }
        PyErr_SetObject((PyObject *) &js_exception_type, exception);
    }
}

void js_throw_py() {
    Local<Context> context = isolate->GetCurrentContext();
    PyObject *exc_type, *exc_value, *exc_traceback;
    PyErr_Fetch(&exc_type, &exc_value, &exc_traceback);
    PyErr_NormalizeException(&exc_type, &exc_value, &exc_traceback);
    Local<Object> exception;
    if (PyObject_TypeCheck(exc_value, &js_exception_type)) {
        exception = ((js_exception *) exc_value)->exception.Get(isolate).As<Object>();
    } else {
        exception = js_from_py(exc_value, context).As<Object>();
        exception->SetInternalField(2, External::New(isolate, exc_type));
        exception->SetInternalField(3, External::New(isolate, exc_traceback));
    }
    isolate->ThrowException(exception);
}
