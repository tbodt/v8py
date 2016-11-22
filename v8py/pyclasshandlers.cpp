#include <v8.h>
#include <Python.h>

#include "v8py.h"
#include "convert.h"
#include "pyclass.h"

void py_class_construct_callback(const FunctionCallbackInfo<Value> &info) {
    HandleScope hs(isolate);
    py_class *self = (py_class *) info.Data().As<External>()->Value();
    Local<Context> context = isolate->GetCurrentContext();

    if (!info.IsConstructCall()) {
        isolate->ThrowException(Exception::TypeError(JSTR("Constructor requires 'new' operator")));
        return;
    }

    Local<Object> js_new_object = info.This();
    PyObject *new_object = PyObject_Call(self->cls, pys_from_jss(info, context), NULL);
    JS_PROPAGATE_PY(new_object);
    py_class_init_js_object(js_new_object, new_object, context);
}

void py_class_method_callback(const FunctionCallbackInfo<Value> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    Local<Object> js_self = info.This();
    PyObject *self = (PyObject *) js_self->GetInternalField(1).As<External>()->Value();
    PyObject *args = pys_from_jss(info, context);
    JS_PROPAGATE_PY(args);
    // add self onto the beginning of the arg list
    PyObject *all_args = PyTuple_New(PyTuple_Size(args) + 1);
    Py_INCREF(self);
    PyTuple_SetItem(all_args, 0, self);
    for (int i = 0; i < PyTuple_Size(args); i++) {
        PyObject *arg = PyTuple_GetItem(args, i);
        Py_INCREF(arg);
        PyTuple_SetItem(all_args, i + 1, arg);
    }
    Py_DECREF(args);

    method_callback_info *callback_info = (method_callback_info *) info.Data().As<External>()->Value();
    PyObject *method = PyObject_GetAttr(callback_info->cls, callback_info->method_name);
    if (method == NULL) {
        Py_DECREF(all_args);
        js_throw_py();
        return;
    }
    assert(PyMethod_Check(method));
    PyObject *retval = PyObject_Call(method, all_args, NULL);
    Py_DECREF(all_args);
    Py_DECREF(method);

    JS_PROPAGATE_PY(retval);
    info.GetReturnValue().Set(js_from_py(retval, context));
    Py_DECREF(retval);
}

// --- Interceptors ---
// TODO everything needs to be fixed with regards to exception handling
// one TODO comment for everything that needs to be fixed

template <class T> inline extern PyObject *get_self(const PropertyCallbackInfo<T> &info) {
    return (PyObject *) info.This()->GetInternalField(1).template As<External>()->Value();
}

void py_class_getter_callback(Local<Name> js_name, const PropertyCallbackInfo<Value> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    PyObject *name = py_from_js(js_name, context);
    JS_PROPAGATE_PY(name);
    if (PyObject_HasAttr(get_self(info), name)) {
        Py_DECREF(name);
        return;
    }
    PyObject *value = PyObject_GetItem(get_self(info), name);
    if (value == NULL) {
        Py_DECREF(name);
        js_throw_py();
        return;
    }
    info.GetReturnValue().Set(js_from_py(value, context));
    Py_DECREF(value);
    Py_DECREF(name);
}

void py_class_setter_callback(Local<Name> js_name, Local<Value> js_value, const PropertyCallbackInfo<Value> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    PyObject *name = py_from_js(js_name, context);
    JS_PROPAGATE_PY(name);
    if (PyObject_HasAttr(get_self(info), name)) {
        Py_DECREF(name);
        return;
    }
    PyObject *value = py_from_js(js_value, context);
    if (value == NULL) {
        Py_DECREF(name);
        js_throw_py();
        return;
    }
    if (PyObject_SetItem(get_self(info), name, value) < 0) {
        Py_DECREF(value);
        Py_DECREF(name);
        js_throw_py();
        return;
    }
    info.GetReturnValue().Set(js_value);
    Py_DECREF(name);
    Py_DECREF(value);
}

void py_class_query_callback(Local<Name> js_name, const PropertyCallbackInfo<Integer> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    PyObject *name = py_from_js(js_name, context);
    if (PyObject_HasAttr(get_self(info), name)) {
        Py_DECREF(name);
        return;
    }

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
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    PyObject *name = py_from_js(js_name, context);
    JS_PROPAGATE_PY(name);
    if (PyObject_HasAttr(get_self(info), name)) {
        Py_DECREF(name);
        return;
    }
    if (PyObject_DelItem(get_self(info), name) < 0) {
        if (info.ShouldThrowOnError()) {
            isolate->ThrowException(Exception::TypeError(JSTR("Unable to delete property.")));
            Py_DECREF(name);
            return;
        }
        info.GetReturnValue().Set(False(isolate));
    } else {
        info.GetReturnValue().Set(True(isolate));
    }
    Py_DECREF(name);
}

void py_class_enumerator_callback(const PropertyCallbackInfo<Array> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    PyObject *keys = PyObject_CallMethod(get_self(info), "keys", "");
    JS_PROPAGATE_PY(keys);
    Local<Array> js_keys = Array::New(isolate, PySequence_Length(keys));
    for (int i = 0; i < PySequence_Length(keys); i++) {
        PyObject *item = PySequence_ITEM(keys, i);
        if (item == NULL) {
            Py_DECREF(keys);
            js_throw_py();
            return;
        }
        js_keys->Set(context, i, js_from_py(item, context)).FromJust();
    }
    info.GetReturnValue().Set(js_keys);
}

void py_class_property_getter(Local<Name> js_name, const PropertyCallbackInfo<Value> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    PyObject *name = py_from_js(js_name, context);
    JS_PROPAGATE_PY(name);
    PyObject *value = PyObject_GetAttr(get_self(info), name);
    JS_PROPAGATE_PY(value);
    info.GetReturnValue().Set(js_from_py(value, context));
}

void py_class_property_setter(Local<Name> js_name, Local<Value> js_value, const PropertyCallbackInfo<void> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    PyObject *name = py_from_js(js_name, context);
    JS_PROPAGATE_PY(name);
    PyObject *value = py_from_js(js_value, context);
    JS_PROPAGATE_PY(value);
    int result = PyObject_SetAttr(get_self(info), name, value);
    JS_PROPAGATE_PY_(value);
}

