#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "pydictionary.h"

using namespace v8;

void py_dictionary_weak_callback(const WeakCallbackInfo<Persistent<Object>> &info);

void py_dictionary_getter_callback(Local<Name> name, const PropertyCallbackInfo<Value> &info);
void py_dictionary_setter_callback(Local<Name> name, Local<Value> value, const PropertyCallbackInfo<Value> &info);
void py_dictionary_query_callback(Local<Name> name, const PropertyCallbackInfo<Integer> &info);
void py_dictionary_deleter_callback(Local<Name> name, const PropertyCallbackInfo<Boolean> &info);
void py_dictionary_enumerator_callback(const PropertyCallbackInfo<Array> &info);

static Persistent<ObjectTemplate> dict_templ;

void py_dictionary_init() {
    IN_V8;

    Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
    templ->SetInternalFieldCount(DICT_INTERNAL_FIELDS);
    templ->SetHandler(NamedPropertyHandlerConfiguration(
                py_dictionary_getter_callback,
                py_dictionary_setter_callback,
                py_dictionary_query_callback,
                py_dictionary_deleter_callback,
                py_dictionary_enumerator_callback));

    dict_templ.Reset(isolate, templ);
}

Local<Object> py_dictionary_get_proxy(PyObject *dict, Local<Context> context) {
    EscapableHandleScope hs(isolate);
    Context::Scope cs(context);
    Local<ObjectTemplate> templ = dict_templ.Get(isolate);
    Local<Object> proxy = templ->NewInstance();

    proxy->SetInternalField(0, IZ_DAT_DICTINARY);
    Py_INCREF(dict);
    proxy->SetInternalField(1, External::New(isolate, dict));

    proxy->SetPrototype(context->GetEmbedderData(1));
    
    // make a weak callback
    Persistent<Object> *weak_handle = new Persistent<Object>(isolate, proxy);
    weak_handle->SetWeak(weak_handle, py_dictionary_weak_callback, WeakCallbackType::kFinalizer);

    return hs.Escape(proxy);
}

void py_dictionary_weak_callback(const WeakCallbackInfo<Persistent<Object>> &info) {
    HandleScope hs(isolate);
    Local<Object> proxy = info.GetParameter()->Get(isolate);
    assert(proxy->GetInternalField(0) == IZ_DAT_DICTINARY);
    PyObject *dict = (PyObject *) proxy->GetInternalField(1).As<External>()->Value();
    assert(PyDict_Check(dict));

    Py_DECREF(dict);

    info.GetParameter()->Reset();
    delete info.GetParameter();
}

template <class T> inline extern PyObject *get_self(const PropertyCallbackInfo<T> &info) {
    assert(info.This()->GetInternalField(0) == IZ_DAT_DICTINARY);
    return (PyObject *) info.This()->GetInternalField(1).template As<External>()->Value();
}

void py_dictionary_getter_callback(Local<Name> js_name, const PropertyCallbackInfo<Value> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    PyObject *self = get_self(info);
    PyObject *name = py_from_js(js_name, context);
    JS_PROPAGATE_PY(name);
    PyObject *value = PyObject_GetItem(self, name);
    Py_DECREF(name);
    JS_PROPAGATE_PY(value);
    info.GetReturnValue().Set(js_from_py(value, context));
}

void py_dictionary_setter_callback(Local<Name> js_name, Local<Value> js_value, const PropertyCallbackInfo<Value> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    PyObject *self = get_self(info);
    PyObject *name = py_from_js(js_name, context);
    JS_PROPAGATE_PY(name);
    PyObject *value = py_from_js(js_value, context);
    Py_DECREF(name);
    JS_PROPAGATE_PY(value);
    int result = PyDict_SetItem(self, name, value);
    JS_PROPAGATE_PY_(result);
    Py_DECREF(value);
}

void py_dictionary_deleter_callback(Local<Name> js_name, const PropertyCallbackInfo<Boolean> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    PyObject *self = get_self(info);
    PyObject *name = py_from_js(js_name, context);
    JS_PROPAGATE_PY(name);
    if (PyObject_DelItem(self, name) < 0) {
        if (info.ShouldThrowOnError()) {
            isolate->ThrowException(Exception::TypeError(JSTR("Unable to delete property.")));
            Py_DECREF(name);
            return;
        }
        info.GetReturnValue().Set(false);
        Py_DECREF(name);
        return;
    }
    info.GetReturnValue().Set(true);
    Py_DECREF(name);
}

void py_dictionary_enumerator_callback(const PropertyCallbackInfo<Array> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    PyObject *self = get_self(info);
    PyObject *keys = PyDict_Keys(self);
    JS_PROPAGATE_PY(keys);
    info.GetReturnValue().Set(js_from_py(keys, context).As<Array>());
}

void py_dictionary_query_callback(Local<Name> js_name, const PropertyCallbackInfo<Integer> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    PyObject *self = get_self(info);
    PyObject *name = py_from_js(js_name, context);
    JS_PROPAGATE_PY(name);
    switch (PyDict_Contains(self, name)) {
        case 1:
            info.GetReturnValue().Set(None);
        case 0:
            break;
        default:
            js_throw_py();
            return;
    }
}

