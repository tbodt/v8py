#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "jsobject.h"

using namespace v8;

PyTypeObject py_js_object_type = {
    PyObject_HEAD_INIT(NULL)
};
PyMethodDef py_js_object_methods[] = {
    {"__dir__", (PyCFunction) py_js_object_dir, METH_NOARGS, NULL},
    {NULL}
};
PyMappingMethods py_js_object_mapping_methods = {
    NULL, (binaryfunc) py_js_object_getattro, (objobjargproc) py_js_object_setattro
};
int py_js_object_type_init() {
    py_js_object_type.tp_name = "v8py.Object";
    py_js_object_type.tp_basicsize = sizeof(py_js_object);
    py_js_object_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_js_object_type.tp_doc = "";

    py_js_object_type.tp_new = py_fake_new;
    py_js_object_type.tp_dealloc = (destructor) py_js_object_dealloc;
    py_js_object_type.tp_getattro = (getattrofunc) py_js_object_getattro;
    py_js_object_type.tp_setattro = (setattrofunc) py_js_object_setattro;
    py_js_object_type.tp_repr = (reprfunc) py_js_object_repr;
    py_js_object_type.tp_methods = py_js_object_methods;
    py_js_object_type.tp_as_mapping = &py_js_object_mapping_methods;
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
        ((py_js_function *) self)->js_this = new Persistent<Value>();
        fprintf(stderr, "%p\n", ((py_js_function *) self)->js_this);
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
    Local<Value> js_name = js_from_py(name, context);

    if (!object->Has(context, js_name).FromJust()) {
        PyObject *class_name = py_from_js(object->GetConstructorName(), context);
        PyErr_Format(PyExc_AttributeError, "'%.50s' JavaScript object has no attribute '%.400s'", 
                PyString_AS_STRING(class_name), PyString_AS_STRING(name));
        Py_DECREF(class_name);
        return NULL;
    }
    PyObject *value = py_from_js(object->Get(context, js_name).ToLocalChecked(), context);
    if (Py_TYPE(value) == &py_js_function_type) {
        py_js_function *func = (py_js_function *) value;
        func->js_this->Reset(isolate, object);
    }
    return value;
}

int py_js_object_setattro(py_js_object *self, PyObject *name, PyObject *value) {
    HandleScope hs(isolate);
    Local<Object> object = self->object->Get(isolate);
    Local<Context> context = self->context->Get(isolate);

    if (!object->Set(context, js_from_py(name, context), js_from_py(value, context)).FromJust()) {
        PyErr_SetString(PyExc_AttributeError, "Object->Set completely failed for some reason");
        return -1;
    }
    return 0;
}

PyObject *py_js_object_dir(py_js_object *self) {
    HandleScope hs(isolate);
    Local<Object> object = self->object->Get(isolate);
    Local<Context> context = self->context->Get(isolate);
    Local<Array> properties = object->GetPropertyNames(context).ToLocalChecked();
    PyObject *py_properties = PyList_New(properties->Length());
    if (py_properties == NULL) {
        return NULL;
    }
    for (int i = 0; i < properties->Length(); i++) {
        PyObject *py_property = py_from_js(properties->Get(context, i).ToLocalChecked(), context);
        PyList_SET_ITEM(py_properties, i, py_property);
    }
    return py_properties;
}

PyObject *py_js_object_repr(py_js_object *self) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Object> object = self->object->Get(isolate);
    Local<Context> context = self->context->Get(isolate);
    return py_from_js(object->ToString(), context);
}

/////////////////////////////////////////////

PyTypeObject py_js_function_type = {
    PyObject_HEAD_INIT(NULL)
};
int py_js_function_type_init() {
    py_js_function_type.tp_name = "v8py.BoundFunction";
    py_js_function_type.tp_basicsize = sizeof(py_js_function);
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
    Local<Value> js_this;
    if (self->js_this->IsEmpty()) {
        js_this = Undefined(isolate);
    } else {
        js_this = self->js_this->Get(isolate);
    }
    int argc;
    Local<Value> *argv = pys_from_jss(args, context, &argc);
    Local<Value> result = object->CallAsFunction(context, js_this, argc, argv).ToLocalChecked();
    return py_from_js(result, context);
}

void py_js_object_dealloc(py_js_object *self) {
    delete self->object;
    delete self->context;
    self->ob_type->tp_free((PyObject *) self);
}

void py_js_function_dealloc(py_js_function *self) {
    delete self->js_this;
    py_js_object_dealloc((py_js_object *) self);
}
