#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "jsobject.h"

using namespace v8;

PyTypeObject js_object_type = {
    PyObject_HEAD_INIT(NULL)
};
PyMethodDef js_object_methods[] = {
    {"__dir__", (PyCFunction) js_object_dir, METH_NOARGS, NULL},
    {NULL}
};
PyMappingMethods js_object_mapping_methods = {
    NULL, (binaryfunc) js_object_getattro, (objobjargproc) js_object_setattro
};
int js_object_type_init() {
    js_object_type.tp_name = "v8py.Object";
    js_object_type.tp_basicsize = sizeof(js_object);
    js_object_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_object_type.tp_doc = "";

    js_object_type.tp_new = py_fake_new;
    js_object_type.tp_dealloc = (destructor) js_object_dealloc;
    js_object_type.tp_getattro = (getattrofunc) js_object_getattro;
    js_object_type.tp_setattro = (setattrofunc) js_object_setattro;
    js_object_type.tp_repr = (reprfunc) js_object_repr;
    js_object_type.tp_methods = js_object_methods;
    js_object_type.tp_as_mapping = &js_object_mapping_methods;
    return PyType_Ready(&js_object_type);
}

PyObject *js_object_fake_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    PyErr_SetString(PyExc_NotImplementedError, "JSObjects can't be created from Python");
    return NULL;
}

js_object *js_object_new(Local<Object> object, Local<Context> context) {
    js_object *self;
    if (object->IsCallable()) {
        self = (js_object *) js_function_type.tp_alloc(&js_function_type, 0);
    } else {
        self = (js_object *) js_object_type.tp_alloc(&js_object_type, 0);
    }

    if (self != NULL) {
        self->object.Reset(isolate, object);
        self->context.Reset(isolate, context);
    }
    return self;
}

PyObject *js_object_getattro(js_object *self, PyObject *name) {
    HandleScope hs(isolate);
    Local<Object> object = self->object.Get(isolate);
    Local<Context> context = self->context.Get(isolate);
    Local<Value> js_name = js_from_py(name, context);

    if (!object->Has(context, js_name).FromJust()) {
        PyObject *class_name = py_from_js(object->GetConstructorName(), context);
        PyErr_Format(PyExc_AttributeError, "'%.50s' JavaScript object has no attribute '%.400s'", 
                PyString_AS_STRING(class_name), PyString_AS_STRING(name));
        Py_DECREF(class_name);
        return NULL;
    }
    PyObject *value = py_from_js(object->Get(context, js_name).ToLocalChecked(), context);
    if (Py_TYPE(value) == &js_function_type) {
        js_function *func = (js_function *) value;
        func->js_this.Reset(isolate, object);
    }
    return value;
}

int js_object_setattro(js_object *self, PyObject *name, PyObject *value) {
    HandleScope hs(isolate);
    Local<Object> object = self->object.Get(isolate);
    Local<Context> context = self->context.Get(isolate);

    if (!object->Set(context, js_from_py(name, context), js_from_py(value, context)).FromJust()) {
        PyErr_SetString(PyExc_AttributeError, "Object->Set completely failed for some reason");
        return -1;
    }
    return 0;
}

PyObject *js_object_dir(js_object *self) {
    HandleScope hs(isolate);
    Local<Object> object = self->object.Get(isolate);
    Local<Context> context = self->context.Get(isolate);
    Local<Array> properties = object->GetOwnPropertyNames(context, ALL_PROPERTIES).ToLocalChecked();
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

PyObject *js_object_repr(js_object *self) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Object> object = self->object.Get(isolate);
    Local<Context> context = self->context.Get(isolate);
    return py_from_js(object->ToString(), context);
}

/////////////////////////////////////////////

PyTypeObject js_function_type = {
    PyObject_HEAD_INIT(NULL)
};
int js_function_type_init() {
    js_function_type.tp_name = "v8py.BoundFunction";
    js_function_type.tp_basicsize = sizeof(js_function);
    js_function_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_function_type.tp_doc = "";
    js_function_type.tp_call = (ternaryfunc) js_function_call;
    js_function_type.tp_base = &js_object_type;
    return PyType_Ready(&js_function_type);
}

PyObject *js_function_call(js_function *self, PyObject *args, PyObject *kwargs) {
    HandleScope hs(isolate);
    Local<Context> context = self->object.context.Get(isolate);

    Local<Object> object = self->object.object.Get(isolate);
    Local<Value> js_this;
    if (self->js_this.IsEmpty()) {
        js_this = Undefined(isolate);
    } else {
        js_this = self->js_this.Get(isolate);
    }
    int argc;
    Local<Value> *argv = pys_from_jss(args, context, &argc);
    Local<Value> result = object->CallAsFunction(context, js_this, argc, argv).ToLocalChecked();
    return py_from_js(result, context);
}

void js_object_dealloc(js_object *self) {
    self->object.Reset();
    self->context.Reset();
    self->ob_type->tp_free((PyObject *) self);
}

void js_function_dealloc(js_function *self) {
    self->js_this.Reset();
    js_object_dealloc((js_object *) self);
}
