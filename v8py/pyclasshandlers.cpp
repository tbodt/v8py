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

void py_class_getter_callback(Local<Name> js_name, const PropertyCallbackInfo<Value> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    Local<Object> js_self = info.This();
    PyObject *self = (PyObject *) js_self->GetInternalField(1).As<External>()->Value();
    PyObject *dict = PyObject_GetAttrString(self, "__dict__");
    // TODO if dict == NULL throw JS exception
    PyObject *name = py_from_js(js_name, context);
    PyObject *value = PyObject_GetItem(dict, name);
    if (value != NULL) {
        info.GetReturnValue().Set(js_from_py(value, context));
    }
    Py_DECREF(dict);
    Py_DECREF(name);
    Py_XDECREF(value);
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

