#include <v8.h>
#include <Python.h>

#include "v8py.h"
#include "convert.h"
#include "pyclass.h"

void py_class_construct_callback(const FunctionCallbackInfo<Value> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    py_class *self = (py_class *) info.Data().As<External>()->Value();
    Local<Context> context = isolate->GetCurrentContext();

    if (!info.IsConstructCall()) {
        // TODO: throw a JS exception
        return;
    }

    Local<Object> js_new_object = info.This();
    PyObject *new_object = PyObject_Call(self->cls, pys_from_jss(info, context), NULL);
    py_class_init_js_object(js_new_object, new_object);
}

void py_class_method_callback(const FunctionCallbackInfo<Value> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    Local<Object> js_self = info.This();
    PyObject *self = (PyObject *) js_self->GetInternalField(1).As<External>()->Value();
    PyObject *args = pys_from_jss(info, context);

    PyObject *method_name = py_from_js(info.Data(), context);
    PyObject *method = PyObject_GetAttr(self, method_name);
    PyObject *retval = PyObject_Call(method, args, NULL);

    info.GetReturnValue().Set(js_from_py(retval, context));

    Py_DECREF(method_name);
    Py_DECREF(method);
    Py_DECREF(args);
    Py_DECREF(retval);
}

// --- Interceptors ---
// TODO everything needs to be fixed with regards to exception handling

template <class T> PyObject *get_self(const PropertyCallbackInfo<T> &info) {
    return (PyObject *) info.This()->GetInternalField(1).template As<External>()->Value();
}

void py_class_getter_callback(Local<Name> js_name, const PropertyCallbackInfo<Value> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    PyObject *name = py_from_js(js_name, context);
    PyObject *value = PyObject_GetItem(get_self(info), name);
    if (value != NULL) {
        info.GetReturnValue().Set(js_from_py(value, context));
        Py_DECREF(value);
    }
    Py_DECREF(name);
}

void py_class_setter_callback(Local<Name> js_name, Local<Value> js_value, const PropertyCallbackInfo<Value> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    PyObject *name = py_from_js(js_name, context);
    PyObject *value = py_from_js(js_value, context);
    if (PyObject_SetItem(get_self(info), name, value) >= 0) {
        info.GetReturnValue().Set(js_value);
    }
    Py_DECREF(name);
    Py_DECREF(value);
}

void py_class_query_callback(Local<Name> js_name, const PropertyCallbackInfo<Integer> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);

    int descriptor = DontEnum;
    PyObject *cls = (PyObject *) Py_TYPE(get_self(info));
    if (!PyObject_HasAttrString(cls, "__setitem__")) {
        descriptor |= ReadOnly;
    }
    if (!PyObject_HasAttrString(cls, "__delitem__")) {
        descriptor |= DontDelete;
    }
    info.GetReturnValue().Set(descriptor);
}

void py_class_deleter_callback(Local<Name> js_name, const PropertyCallbackInfo<Boolean> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    PyObject *name = py_from_js(js_name, context);
    if (PyObject_DelItem(get_self(info), name) >= 0) {
        info.GetReturnValue().Set(True(isolate));
    } else {
        info.GetReturnValue().Set(False(isolate));
    }
    Py_DECREF(name);
}

void py_class_enumerator_callback(const PropertyCallbackInfo<Array> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    PyObject *keys = PyObject_CallMethod(get_self(info), "keys", "");
    Local<Array> js_keys = Array::New(isolate, PySequence_Length(keys));
    for (int i = 0; i < PySequence_Length(keys); i++) {
        js_keys->Set(context, i, js_from_py(PySequence_ITEM(keys, i), context)).FromJust();
    }
    info.GetReturnValue().Set(js_keys);
}

