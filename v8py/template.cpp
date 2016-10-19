#include <stdio.h>
#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "context.h"
#include "convert.h"
#include "template.h"

PyTypeObject py_template_type = {
    PyObject_HEAD_INIT(NULL)
};

int py_template_type_init() {
    py_template_type.tp_name = "v8py.FunctionTemplate";
    py_template_type.tp_basicsize = sizeof(py_template);
    py_template_type.tp_dealloc = (destructor) py_template_dealloc;
    py_template_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_template_type.tp_doc = "";
    py_template_type.tp_init = (initproc) py_template_init;
    py_template_type.tp_new = (newfunc) py_template_new;
    return PyType_Ready(&py_template_type);
}

static void py_template_callback(const FunctionCallbackInfo<Value> &info);

PyObject *py_template_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    py_template *self = (py_template *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->js_template = new Persistent<FunctionTemplate>();
    }
    return (PyObject *) self;
}

PyObject *py_template_init(py_template *self, PyObject *args, PyObject *kwargs) {
    PyObject *function;
    if (!PyArg_ParseTuple(args, "O:__init__", &function)) {
        return NULL;
    }
    if (!PyCallable_Check(function)) {
        PyErr_SetString(PyExc_TypeError, "FunctionTemplate must be passed a function");
        return NULL;
    }
    Py_INCREF(function);
    self->function = function;

    // I've discovered that v8 trades memory leaks for speed. If you allocate a
    // FunctionTemplate and instantiate it, the FunctionTemplate, callback
    // data, and instantiated Function will **never** get GC'd. Presumably,
    // this is by design. This means that the FunctionTemplate python object
    // should never be freed either.
    Py_INCREF(self);

    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    Local<External> js_self = External::New(isolate, self);
    Local<FunctionTemplate> js_template = FunctionTemplate::New(isolate, py_template_callback, js_self);
    self->js_template->Reset(isolate, js_template);

    return NULL;
}

Local<Function> py_template_to_function(py_template *self, Local<Context> context) {
    EscapableHandleScope hs(isolate);
    Local<Function> function = self->js_template->Get(isolate)->GetFunction(context).ToLocalChecked();
    return hs.Escape(function);
}

static void py_template_callback(const FunctionCallbackInfo<Value> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    py_template *self = (py_template *) info.Data().As<External>()->Value();
    PyObject *args = PyTuple_New(info.Length());
    for (int i = 0; i < info.Length(); i++) {
        PyTuple_SET_ITEM(args, i, py_from_js(info[i], context));
    }
    PyObject *result = PyObject_CallObject(self->function, args);
    if (result == NULL) {
        // function threw an exception
        // TODO throw an actual javascript exception
        printf("exception was thrown");
        Py_INCREF(Py_None);
        result = Py_None;
    }
    Py_DECREF(args);
    Local<Value> js_result = js_from_py(result);
    Py_DECREF(result);
    info.GetReturnValue().Set(js_result);
}

void py_template_dealloc(py_template *self) {
    printf("this should never happen\n");
    (void) *((char *)NULL); // intentional segfault
}

