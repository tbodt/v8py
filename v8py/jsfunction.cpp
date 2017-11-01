#include <Python.h>
#include <v8.h>
#include <pthread.h>

#include "v8py.h"
#include "jsobject.h"
#include "convert.h"
#include "context.h"

PyTypeObject js_function_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
int js_function_type_init() {
    js_function_type.tp_name = "v8py.BoundFunction";
    js_function_type.tp_basicsize = sizeof(js_function);
    js_function_type.tp_dealloc = (destructor) js_function_dealloc;
    js_function_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_function_type.tp_doc = "";
    js_function_type.tp_call = (ternaryfunc) js_function_call;
    js_function_type.tp_base = &js_object_type;
    return PyType_Ready(&js_function_type);
}

PyObject *js_function_call(js_function *self, PyObject *args, PyObject *kwargs) {
    IN_V8;
    IN_CONTEXT(self->context.Get(isolate));
    JS_TRY

    Local<Object> object = self->object.Get(isolate);
    Local<Value> js_this;
    if (self->js_this.IsEmpty()) {
        js_this = Undefined(isolate);
    } else {
        js_this = self->js_this.Get(isolate);
    }
    int argc = PyTuple_GET_SIZE(args);
    Local<Value> *argv = new Local<Value>[argc];
    jss_from_pys(args, argv, context);

    double timeout = 0;
    if (kwargs) {
        PyObject *timeoutObj = PyDict_GetItemString(kwargs, "timeout");
        if (timeoutObj) {
            if (!PyFloat_Check(timeoutObj)) {
                PyErr_SetString(PyExc_TypeError, "timeout argument must be a float");
                return NULL;
            }
            timeout = PyFloat_AS_DOUBLE(timeoutObj);
        }
    }

    pthread_t breaker_id;
    if (timeout > 0) {
        errno = pthread_create(&breaker_id, NULL, breaker_thread, &timeout);
        if (errno) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }
    }

    MaybeLocal<Value> result = object->CallAsFunction(context, js_this, argc, argv);

    if (timeout > 0) {
        pthread_cancel(breaker_id);
        errno = pthread_join(breaker_id, NULL);
        if (errno) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }
    }

    delete[] argv;
    PY_PROPAGATE_JS;
    return py_from_js(result.ToLocalChecked(), context);
}

void js_function_dealloc(js_function *self) {
    self->js_this.Reset();
    js_object_dealloc((js_object *) self);
}
