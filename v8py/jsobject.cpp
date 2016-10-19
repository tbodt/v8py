#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "jsobject.h"

using namespace v8;

PyTypeObject py_js_object_type = {
    PyObject_HEAD_INIT(NULL)
};
int py_js_object_type_init() {
    py_js_object_type.tp_name = "v8py.JSObject";
    py_js_object_type.tp_basicsize = sizeof(py_js_object);
    py_js_object_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_js_object_type.tp_doc = "";

    py_js_object_type.tp_new = (newfunc) py_js_object_fake_new;
    py_js_object_type.tp_dealloc = (destructor) py_js_object_dealloc;
    py_js_object_type.tp_getattro = (getattrofunc) py_js_object_getattro;
    py_js_object_type.tp_setattro = (setattrofunc) py_js_object_setattro;
    return PyType_Ready(&py_js_object_type);
}

PyObject *py_js_object_fake_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    PyErr_SetString(PyExc_NotImplementedError, "JSObjects can't be created from Python");
    return NULL;
}

py_js_object *py_js_object_new(Local<Object> object, Local<Context> context) {
    py_js_object *self;
    if (object->IsCallable()) {
        self = (py_js_object *) py_js_function_type.tp_alloc(&py_js_function_type, 0);
    } else {
        self = (py_js_object *) py_js_object_type.tp_alloc(&py_js_object_type, 0);
    }

    if (self != NULL) {
        self->object = new Persistent<Object>(isolate, object);
        self->context = new Persistent<Context>(isolate, context);
    }
    return self;
}

PyObject *py_js_object_getattro(py_js_object *self, PyObject *name) {
    HandleScope hs(isolate);
    Local<Object> object = self->object->Get(isolate);
    Local<Context> context = self->context->Get(isolate);
    Local<Value> js_name = js_from_py(name);

    if (!object->Has(context, js_name).FromJust()) {
        PyObject *class_name = py_from_js(object->GetConstructorName(), context);
        PyErr_Format(PyExc_AttributeError, "'%.50s' JavaScript object has no attribute '%.400s'", 
                PyString_AS_STRING(class_name), PyString_AS_STRING(name));
        Py_DECREF(class_name);
        return NULL;
    }
    return py_from_js(object->Get(context, js_name).ToLocalChecked(), context);
}

int py_js_object_setattro(py_js_object *self, PyObject *name, PyObject *value) {
    HandleScope hs(isolate);
    Local<Object> object = self->object->Get(isolate);
    Local<Context> context = self->context->Get(isolate);

    if (!object->Set(context, js_from_py(name), js_from_py(value)).FromJust()) {
        PyErr_SetString(PyExc_AttributeError, "Object->Set completely failed for some reason");
        return -1;
    }
    return 0;
}

PyTypeObject py_js_function_type = {
    PyObject_HEAD_INIT(NULL)
};
int py_js_function_type_init() {
    py_js_function_type.tp_name = "v8py.JSFunction";
    py_js_function_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_js_function_type.tp_doc = "";
    py_js_function_type.tp_call = (ternaryfunc) py_js_function_call;
    py_js_function_type.tp_base = &py_js_object_type;
    return PyType_Ready(&py_js_function_type);
}

PyObject *py_js_function_call(py_js_function *self, PyObject *args, PyObject *kwargs) {
    HandleScope hs(isolate);
    Local<Context> context = self->object.context->Get(isolate);

    Local<Object> object = self->object.object->Get(isolate);
    int argc = PyTuple_GET_SIZE(args);
    Local<Value> *argv = new Local<Value>[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = js_from_py(PyTuple_GET_ITEM(args, i));
    }
    Local<Value> result = object->CallAsFunction(context, Undefined(isolate), argc, argv).ToLocalChecked();
    return py_from_js(result, context);
}

void py_js_object_dealloc(py_js_object *self) {
    delete self->object;
    delete self->context;
}
