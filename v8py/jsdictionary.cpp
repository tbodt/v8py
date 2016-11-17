#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "jsobject.h"
#include "convert.h"

PyTypeObject js_dictionary_type = {
    PyObject_HEAD_INIT(NULL)
};
PyMethodDef js_dictionary_methods[] = {
    {"keys", (PyCFunction) js_dictionary_keys, METH_NOARGS, NULL},
    {NULL}
};
PyMappingMethods js_dictionary_mapping_methods = {
    (lenfunc) js_dictionary_length,
    (binaryfunc) js_dictionary_getitem, 
    (objobjargproc) js_dictionary_setitem,
};
int js_dictionary_type_init() {
    js_dictionary_type.tp_name = "v8py.Dictionary";
    js_dictionary_type.tp_basicsize = sizeof(js_dictionary);
    js_dictionary_type.tp_base = &js_object_type;
    js_dictionary_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_dictionary_type.tp_doc = "";

    js_dictionary_type.tp_methods = js_dictionary_methods;
    js_dictionary_type.tp_as_mapping = &js_dictionary_mapping_methods;
    return PyType_Ready(&js_dictionary_type);
}

PyObject *js_dictionary_keys(js_dictionary *self) {
    HandleScope hs(isolate);
    Local<Context> context = self->context.Get(isolate);
    Local<Object> object = self->object.Get(isolate);

    Local<Array> properties = object->GetOwnPropertyNames(context, ONLY_ENUMERABLE).ToLocalChecked();
    return py_from_js(properties, context);
}

Py_ssize_t js_dictionary_length(js_dictionary *self) {
    PyObject *keys = js_dictionary_keys(self);
    Py_ssize_t keys_size = PySequence_Length(keys);
    Py_DECREF(keys);
    return keys_size;
}

PyObject *js_dictionary_getitem(js_dictionary *self, PyObject *key) {
    HandleScope hs(isolate);
    Local<Context> context = self->context.Get(isolate);
    Local<Object> object = self->object.Get(isolate);
    Local<String> js_key = js_from_py(key, context).As<String>();
    if (!object->Has(context, js_key).FromJust()) {
        PyErr_Format(PyExc_KeyError, "%s", PyString_AS_STRING(key));
        return NULL;
    }
    return py_from_js(object->Get(context, js_key).ToLocalChecked(), context);
}

int js_dictionary_setitem(js_dictionary *self, PyObject *key, PyObject *value) {
    HandleScope hs(isolate);
    Local<Context> context = self->context.Get(isolate);
    Local<Object> object = self->object.Get(isolate);
    Local<String> js_key = js_from_py(key, context).As<String>();
    if (value != NULL) {
        Local<Value> js_value = js_from_py(value, context);
        if (!object->Set(context, js_key, js_value).FromJust()) {
            // TODO
            return -1;
        }
    } else {
        // Python implements delitem by calling setitem with a NULL value.
        if (!object->Delete(context, js_key).FromJust()) {
            // TODO
            return -1;
        }
    }
    return 0;
}

