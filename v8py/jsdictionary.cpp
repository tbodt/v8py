#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "jsobject.h"
#include "convert.h"

PyTypeObject js_dictionary_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
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

    js_dictionary_type.tp_repr = (reprfunc) js_dictionary_repr;
    js_dictionary_type.tp_str = (reprfunc) js_dictionary_repr;
    js_dictionary_type.tp_methods = js_dictionary_methods;
    js_dictionary_type.tp_as_mapping = &js_dictionary_mapping_methods;
    js_dictionary_type.tp_iter = (getiterfunc) js_dictionary_iter;
    return PyType_Ready(&js_dictionary_type);
}

PyObject *js_dictionary_keys(js_dictionary *self) {
    IN_V8;
    IN_CONTEXT(self->context.Get(isolate));
    Local<Object> object = self->object.Get(isolate);
    JS_TRY

    MaybeLocal<Array> properties = object->GetOwnPropertyNames(context, ONLY_ENUMERABLE);
    PY_PROPAGATE_JS;
    return py_from_js(properties.ToLocalChecked(), context);
}

Py_ssize_t js_dictionary_length(js_dictionary *self) {
    PyObject *keys = js_dictionary_keys(self);
    Py_ssize_t keys_size = PySequence_Length(keys);
    Py_DECREF(keys);
    return keys_size;
}

PyObject *js_dictionary_getitem(js_dictionary *self, PyObject *key) {
    IN_V8;
    IN_CONTEXT(self->context.Get(isolate));
    Local<Object> object = self->object.Get(isolate);
    JS_TRY

    Local<String> js_key = js_from_py(key, context).As<String>();
    if (!object->Has(context, js_key).FromJust()) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    MaybeLocal<Value> value = object->Get(context, js_key);
    PY_PROPAGATE_JS;
    return py_from_js(value.ToLocalChecked(), context);
}

int js_dictionary_setitem(js_dictionary *self, PyObject *key, PyObject *value) {
    IN_V8;
    IN_CONTEXT(self->context.Get(isolate));
    Local<Object> object = self->object.Get(isolate);
    Local<String> js_key = js_from_py(key, context).As<String>();
    JS_TRY

    if (value != NULL) {
        Local<Value> js_value = js_from_py(value, context);
        Maybe<bool> result = object->Set(context, js_key, js_value);
        PY_PROPAGATE_JS_;
    } else {
        // Python implements delitem by calling setitem with a NULL value.
        Maybe<bool> result = object->Delete(context, js_key);
        PY_PROPAGATE_JS_;
    }
    return 0;
}

PyObject *js_dictionary_iter(js_dictionary *self) {
    // Implemented as iter(self.keys())
    PyObject *keys = js_dictionary_keys(self);
    PyErr_PROPAGATE(keys);
    PyObject *keys_iter = PyObject_GetIter(keys);
    Py_DECREF(keys);
    return keys_iter;
}

PyObject *js_dictionary_repr(js_dictionary *self) {
    PyObject *args = Py_BuildValue("(O)", self);
    PyErr_PROPAGATE(args);
    PyObject *dict = PyObject_Call((PyObject *) &PyDict_Type, args, NULL);
    Py_DECREF(args);
    PyErr_PROPAGATE(dict);
    return PyObject_Repr(dict);
}

