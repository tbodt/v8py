#include <Python.h>
#include "v8py.h"
#include <v8.h>

#include "convert.h"
#include "jsobject.h"
#include "context.h"

using namespace v8;

PyTypeObject js_object_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
PyMethodDef js_object_methods[] = {
    {"__dir__", (PyCFunction) js_object_dir, METH_NOARGS, NULL},
    {"to_bytes", (PyCFunction) js_object_to_bytes, METH_NOARGS, NULL},
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

    js_object_type.tp_dealloc = (destructor) js_object_dealloc;
    js_object_type.tp_getattro = (getattrofunc) js_object_getattro;
    js_object_type.tp_setattro = (setattrofunc) js_object_setattro;
    js_object_type.tp_iter = (getiterfunc) js_object_getiter;
    js_object_type.tp_repr = (reprfunc) js_object_repr;
    js_object_type.tp_methods = js_object_methods;
    js_object_type.tp_as_mapping = &js_object_mapping_methods;
    return PyType_Ready(&js_object_type);
}

PyTypeObject js_promise_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
int js_promise_type_init() {
    js_promise_type.tp_name = "v8py.Promise";
    js_promise_type.tp_basicsize = sizeof(js_promise);
    js_promise_type.tp_dealloc = (destructor) js_promise_dealloc;
    js_promise_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_promise_type.tp_doc = "";
    js_promise_type.tp_base = &js_object_type;
    return PyType_Ready(&js_promise_type);
}

js_object *js_object_new(Local<Object> object, Local<Context> context) {
    IN_V8;
    Context::Scope cs(context);
    js_object *self;
    if (object->IsPromise()) {
        self = (js_object *) js_promise_type.tp_alloc(&js_promise_type, 0);
    } else if (object->IsCallable()) {
        self = (js_object *) js_function_type.tp_alloc(&js_function_type, 0);
    } else {
        self = (js_object *) js_object_type.tp_alloc(&js_object_type, 0);
    }

    if (self != NULL) {
        self->object.Reset(isolate, object);
    }
    return self;
}

void js_object_weak_callback(const WeakCallbackInfo<js_object> &info) {
    js_object *self = info.GetParameter();
    self->object.Reset();

    Py_DECREF(self);
}

js_object *js_object_weak_new(Local<Object> object, Local<Context> context) {
    IN_V8;
    Context::Scope cs(context);
    js_object *self;
    if (object->IsPromise()) {
        self = (js_object *) js_promise_type.tp_alloc(&js_promise_type, 0);
    } else if (object->IsCallable()) {
        self = (js_object *) js_function_type.tp_alloc(&js_function_type, 0);
    } else {
        self = (js_object *) js_object_type.tp_alloc(&js_object_type, 0);
    }


    if (self != NULL) {
        self->object.Reset(isolate, object);
        self->object.SetWeak(self, js_object_weak_callback, WeakCallbackType::kFinalizer);
    }
    return self;
}

PyObject *js_object_getattro(js_object *self, PyObject *name) {
    if (PyObject_GenericHasAttr((PyObject *) self, name)) {
        return PyObject_GenericGetAttr((PyObject *) self, name);
    }
    IN_V8;
    Local<Object> object = self->object.Get(isolate);
    IN_CONTEXT(object->CreationContext());
    Local<Value> js_name = js_from_py(name, context);
    JS_TRY
    if (!context_setup_timeout(context)) return NULL;
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
    PY_PROPAGATE_JS;

    MaybeLocal<Value> js_value = object->Get(context, js_name);
    if (!context_cleanup_timeout(context)) return NULL;

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
    Local<Object> object = self->object.Get(isolate);
    IN_CONTEXT(object->CreationContext());
    JS_TRY


    if (!context_setup_timeout(context)) return -1;

    if (value != NULL)
        object->Set(context, js_from_py(name, context), js_from_py(value, context));
    else
        object->Delete(context, js_from_py(name, context));

    if (!context_cleanup_timeout(context)) return -1;

    PY_PROPAGATE_JS_RET(-1);
    return 0;
}

PyObject *js_object_getiter(js_object *self) {
    PyObject *props = js_object_dir(self);
    PyErr_PROPAGATE(props);
    PyObject *iter = PySeqIter_New(props);
    Py_DECREF(props);
    return iter;
}

PyObject *js_object_dir(js_object *self) {
    IN_V8;
    Local<Object> object = self->object.Get(isolate);
    Local<Context> context = object->CreationContext();
    Context::Scope cs(context);
    JS_TRY

    MaybeLocal<Array> maybe_properties = object->GetOwnPropertyNames(context, ALL_PROPERTIES);
    PY_PROPAGATE_JS;
    Local<Array> properties = maybe_properties.ToLocalChecked();
    PyObject *py_properties = PyList_New(properties->Length());
    PyErr_PROPAGATE(py_properties);

    if (!context_setup_timeout(context)) return NULL;
    for (unsigned i = 0; i < properties->Length(); i++) {
        MaybeLocal<Value> js_property = properties->Get(context, i);
        PY_PROPAGATE_JS;
        PyObject *py_property = py_from_js(js_property.ToLocalChecked(), context);
        PyList_SET_ITEM(py_properties, i, py_property);
    }

    if (!context_cleanup_timeout(context)) return NULL;

    return py_properties;
}

PyObject *js_object_to_bytes(js_object *self) {
    IN_V8;
    Local<Object> object = self->object.Get(isolate);
    Local<Context> context = object->CreationContext();
    Context::Scope cs(context);

    if (!object->IsTypedArray()) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    Local<TypedArray> array = object.As<TypedArray>();
    size_t length = array->Length();
    PyObject *bytes = PyBytes_FromStringAndSize(nullptr, length);
    Py_buffer view;
    int error = PyObject_GetBuffer(bytes, &view, PyBUF_SIMPLE);
    array->CopyContents(view.buf, view.len);
    PyBuffer_Release(&view);
    return bytes;
}

PyObject *js_object_repr(js_object *self) {
    IN_V8;
    Local<Object> object = self->object.Get(isolate);
    Local<Context> context = object->CreationContext();
    Context::Scope cs(context);

    return py_from_js(object->ToString(), context);
}

void js_object_dealloc(js_object *self) {
    self->object.Reset();
    Py_TYPE(self)->tp_free((PyObject *) self);
}

void js_promise_dealloc(js_promise *self) {
    js_object_dealloc((js_object *) self);
}

