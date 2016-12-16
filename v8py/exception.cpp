#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "pyclass.h"
#include "convert.h"

PyTypeObject js_exception_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
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

    PyObject *py_message = py_from_js(exception->ToString(), no_ctx);
    PyErr_PROPAGATE(py_message);
#if PY_MAJOR_VERSION < 3
    self->base.message = py_message;
    Py_INCREF(self->base.message);
#endif
    self->base.args = PyTuple_New(1);
    PyErr_PROPAGATE(self->base.args);
    PyTuple_SetItem(self->base.args, 0, py_message);
    return (PyObject *) self;
}

void js_exception_dealloc(js_exception *self) {
    self->exception.Reset();
    self->message.Reset();
    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyTypeObject js_terminated_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
int js_terminated_type_init() {
    js_terminated_type.tp_name = "v8py.JavaScriptTerminated";
    js_terminated_type.tp_base = (PyTypeObject *) PyExc_Exception;
    js_terminated_type.tp_basicsize = sizeof(PyBaseExceptionObject);
    js_terminated_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_terminated_type.tp_doc = "";
    return PyType_Ready(&js_terminated_type);
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
    PyErr_Print();
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
