#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "jsobject.h"
#include "context.h"

using namespace v8;

PyTypeObject js_object_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
PyMethodDef js_object_methods[] = {
    {"__dir__", (PyCFunction) js_object_dir, METH_NOARGS, NULL},
    {NULL}
};
PyMappingMethods js_object_mapping_methods = {
    (lenfunc) js_object_length, (binaryfunc) js_object_getattro, (objobjargproc) js_object_setattro
};
int js_object_type_init() {
    js_object_type.tp_name = "v8py.Object";
    js_object_type.tp_basicsize = sizeof(js_object);
    js_object_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_object_type.tp_doc = "";

    js_object_type.tp_dealloc = (destructor) js_object_dealloc;
    js_object_type.tp_getattro = (getattrofunc) js_object_getattro;
    js_object_type.tp_setattro = (setattrofunc) js_object_setattro;
    js_object_type.tp_repr = (reprfunc) js_object_repr;
    js_object_type.tp_methods = js_object_methods;
    js_object_type.tp_as_mapping = &js_object_mapping_methods;
    return PyType_Ready(&js_object_type);
}

js_object *js_object_new(Local<Object> object, Local<Context> context) {
    IN_V8;
    Context::Scope cs(context);
    js_object *self;
    if (object->GetPrototype()->StrictEquals(context->GetEmbedderData(OBJECT_PROTOTYPE_SLOT))) {
        self = (js_object *) js_dictionary_type.tp_alloc(&js_dictionary_type, 0);
    } else if (object->IsCallable()) {
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
    if (PyObject_GenericHasAttr((PyObject *) self, name)) {
        return PyObject_GenericGetAttr((PyObject *) self, name);
    }

    IN_V8;
    Local<Object> object = self->object.Get(isolate);
    IN_CONTEXT(self->context.Get(isolate));
    Local<Value> js_name = js_from_py(name, context);
    JS_TRY

    if (!object->Has(context, js_name).FromJust()) {
        // TODO fix this so that it works
        PyObject *class_name = py_from_js(object->GetConstructorName(), context);
        PyErr_PROPAGATE(class_name);
        PyObject *class_name_string = PyObject_Str(class_name);
        Py_DECREF(class_name);
        PyErr_PROPAGATE(class_name_string);
#if PY_MAJOR_VERSION < 3
        PyErr_Format(PyExc_AttributeError, "'%.50s' JavaScript object has no attribute '%.400s'", 
                PyString_AS_STRING(class_name_string), PyString_AS_STRING(name));
#else
        PyErr_Format(PyExc_AttributeError, "'%.50S' JavaScript object has no attribute '%.400S'", 
                class_name_string, name);
#endif
        Py_DECREF(class_name_string);
        return NULL;
    }
    MaybeLocal<Value> js_value = object->Get(context, js_name);
    PY_PROPAGATE_JS;
    PyObject *value = py_from_js(js_value.ToLocalChecked(), context);
    PyErr_PROPAGATE(value);
    // if this was called like object.method() then bind the return value to make it callable
    if (Py_TYPE(value) == &js_function_type) {
        js_function *func = (js_function *) value;
        func->js_this.Reset(isolate, object);
    }
    return value;
}

int js_object_setattro(js_object *self, PyObject *name, PyObject *value) {
    if (PyObject_GenericHasAttr((PyObject *) self, name)) {
        return PyObject_GenericSetAttr((PyObject *) self, name, value);
    }

    IN_V8;
    IN_CONTEXT(self->context.Get(isolate));

    Local<Object> object = self->object.Get(isolate);
    if (!object->Set(context, js_from_py(name, context), js_from_py(value, context)).FromJust()) {
        PyErr_SetString(PyExc_AttributeError, "Object->Set completely failed for some reason");
        return -1;
    }
    return 0;
}

static PyObject *length_name = PyString_InternFromString("length");

Py_ssize_t js_object_length(js_object *self) {
    PyObject *length_obj = js_object_getattro(self, length_name);
    Py_DECREF(length_name);
    PyErr_PROPAGATE_(length_obj);
    Py_ssize_t length = PyNumber_AsSsize_t(length_obj, NULL);
    Py_DECREF(length_obj);
    return length;
}

PyObject *js_object_dir(js_object *self) {
    IN_V8;
    Local<Context> context = self->context.Get(isolate);
    Context::Scope cs(context);
    JS_TRY

    Local<Object> object = self->object.Get(isolate);
    MaybeLocal<Array> maybe_properties = object->GetOwnPropertyNames(context, ALL_PROPERTIES);
    PY_PROPAGATE_JS;
    Local<Array> properties = maybe_properties.ToLocalChecked();
    PyObject *py_properties = PyList_New(properties->Length());
    PyErr_PROPAGATE(py_properties);
    for (unsigned i = 0; i < properties->Length(); i++) {
        MaybeLocal<Value> js_property = properties->Get(context, i);
        PY_PROPAGATE_JS;
        PyObject *py_property = py_from_js(js_property.ToLocalChecked(), context);
        PyList_SET_ITEM(py_properties, i, py_property);
    }
    return py_properties;
}

PyObject *js_object_repr(js_object *self) {
    IN_V8;
    Local<Context> context = self->context.Get(isolate);
    Context::Scope cs(context);

    Local<Object> object = self->object.Get(isolate);
    return py_from_js(object->ToString(), context);
}

void js_object_dealloc(js_object *self) {
    self->object.Reset();
    self->context.Reset();
    Py_TYPE(self)->tp_free((PyObject *) self);
}

