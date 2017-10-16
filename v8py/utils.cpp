
#include "utils.h"
#include "v8py.h"
#include "jsobject.h"
#include "convert.h"

using namespace v8;

PyObject *construct_new_object(PyObject *self, PyObject *args) {

    int argc = (int)PyTuple_GET_SIZE(args);
    if (argc == 0) {
        PyErr_SetString(PyExc_TypeError, "First argument must be a constructor function.");
        return NULL;
    }

    PyObject *constructor = PyTuple_GET_ITEM(args, 0);
    if (!PyObject_TypeCheck(constructor, &js_function_type)) {
        PyErr_SetString(PyExc_TypeError, "First argument must be a constructor function.");
        return NULL;
    }

    js_function *function = (js_function*)constructor;

    IN_V8;
    IN_CONTEXT(function->context.Get(isolate))
    JS_TRY

    Local<Object> object = function->object.Get(isolate);
    if (!object->IsConstructor()) {
        PyErr_SetString(PyExc_TypeError, "First argument must be a constructor function.");
        return NULL;
    }

    // exclude first argument
    argc--;

    Local<Value> *argv = new Local<Value>[argc];
    for (long i = 0; i < argc; i++) {
        argv[i] = js_from_py(PyTuple_GET_ITEM(args, i + 1), context);
    }
    MaybeLocal<Value> result = object->CallAsConstructor(argc, argv);
    delete[] argv;
    PY_PROPAGATE_JS;

    return py_from_js(result.ToLocalChecked(), context);
}