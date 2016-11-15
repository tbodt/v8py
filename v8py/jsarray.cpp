#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "jsobject.h"
#include "convert.h"

PySequenceMethods js_array_sequence;
PyTypeObject js_array_type = {
    PyObject_HEAD_INIT(NULL)
};
int js_array_type_init() {
    js_array_sequence.sq_length = (lenfunc) js_array_length;
    js_array_sequence.sq_concat = (binaryfunc) js_array_concat;
    js_array_sequence.sq_repeat = (ssizeargfunc) js_array_repeat;
    js_array_sequence.sq_item = (ssizeargfunc) js_array_getitem;
    js_array_sequence.sq_ass_item = (ssizeobjargproc) js_array_setitem;

    js_array_type.tp_name = "v8py.Array";
    js_array_type.tp_basicsize = sizeof(js_array);
    js_array_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_array_type.tp_doc = "";
    js_array_type.tp_base = &js_object_type;
    js_array_type.tp_as_sequence = &js_array_sequence;
    return PyType_Ready(&js_array_type);
}

Py_ssize_t js_array_length(js_array *self) {
    HandleScope hs(isolate);
    return self->array.Get(isolate)->Length();
}

PyObject *js_array_concat(js_array *self, PyObject *other) {
    PyObject *self_list = PySequence_List((PyObject *) self);
    if (self_list == NULL) {
        return NULL;
    }
    return PySequence_Concat(self_list, other);
}

PyObject *js_array_repeat(js_array *self, Py_ssize_t count) {
    PyObject *self_list = PySequence_List((PyObject *) self);
    if (self_list == NULL) {
        return NULL;
    }
    return PySequence_Repeat(self_list, count);
}

PyObject *js_array_getitem(js_array *self, Py_ssize_t i) {
    HandleScope hs(isolate);
    Local<Array> array = self->array.Get(isolate);
    Local<Context> context = self->context.Get(isolate);
    if (i >= js_array_length(self)) {
        PyErr_SetString(PyExc_IndexError, "JavaScript array index out of range");
        return NULL;
    }
    return py_from_js(array->Get(context, i).ToLocalChecked(), context);
}

int js_array_setitem(js_array *self, Py_ssize_t i, PyObject *value) {
    HandleScope hs(isolate);
    Local<Array> array = self->array.Get(isolate);
    Local<Context> context = self->context.Get(isolate);
    if (i >= js_array_length(self)) {
        PyErr_SetString(PyExc_IndexError, "JavaScript array index out of range");
        return -1;
    }
    if (!array->Set(context, i, js_from_py(value, context)).FromJust()) {
        PyErr_SetString(PyExc_KeyError, "attempt to assign to readonly JavaScript index");
        return -1;
    }
    return 0;
}

