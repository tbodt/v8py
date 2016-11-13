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

PyObject *get_self(const PropertyCallbackInfo<Value> &info) {
    return (PyObject *) info.This()->GetInternalField(1).As<External>()->Value();
}

// Returns whether the property is provided by the object's class, as opposed to the object (via __dict__, __getattr__, etc.).
// Basically, true for methods and class properties.
// If false, interceptors should not do anything and let default behavior happen.
// TODO return false for descriptors, maybe? or maybe specific handlers override general handlers?
int is_own_property(PyObject *object, PyObject *name) {
    PyObject *type;
    if (PyInstance_Check(object)) {
        type = PyObject_GetAttrString(object, "__class__");
    } else {
        type = (PyObject *) Py_TYPE(object);
        Py_INCREF(type);
    }
    int result = !PyObject_HasAttr(type, name);
    Py_DECREF(type);
    return result;
}

void py_class_getter_callback(Local<Name> js_name, const PropertyCallbackInfo<Value> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    PyObject *name = py_from_js(js_name, context);
    PyObject *self = get_self(info);
    if (is_own_property(self, name)) {
        PyObject *value = PyObject_GetItem(self, name);
        if (value != NULL && value != Py_None) {
            info.GetReturnValue().Set(js_from_py(value, context));
            Py_DECREF(value);
        } else {
            value = PyObject_GetAttr(self, name);
            if (value != NULL) {
                info.GetReturnValue().Set(js_from_py(value, context));
                Py_DECREF(value);
            }
        }
    }
    Py_DECREF(name);
}

void py_class_setter_callback(Local<Name> name, Local<Value> value, const PropertyCallbackInfo<Value> &info) {
    printf("setter\n");
}

void py_class_query_callback(Local<Name> name, const PropertyCallbackInfo<Integer> &info) {
    printf("query\n");
}

void py_class_deleter_callback(Local<Name> name, const PropertyCallbackInfo<Boolean> &info) {
    printf("deleter\n");
}

void py_class_enumerator_callback(const PropertyCallbackInfo<Array> &info) {
    printf("enumerator\n");
}

