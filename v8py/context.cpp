#include <Python.h>
#include <v8.h>

#include "context.h"
#include "template.h"
#include "convert.h"
#include "v8py.h"

using namespace v8;

template <class T> static Local<T> unwrap_local(MaybeLocal<T> local) {
    // FIXME: make this, I don't know, throw a python exception?
    return local.ToLocalChecked();
}

PyMethodDef py_context_methods[] = {
    {"eval", (PyCFunction) py_context_eval, METH_O, NULL},
    {"gc", (PyCFunction) py_context_gc, METH_NOARGS, NULL},
    {NULL},
};

// Python is wrong. The first entry is not modifiable and should be const char *
PyGetSetDef py_context_getset[] = {
    {(char *) "glob", (getter) py_context_get_global, NULL, NULL, NULL},
    {NULL},
};

PyTypeObject py_context_type = {
    PyObject_HEAD_INIT(NULL)
};

int py_context_type_init() {
    py_context_type.tp_name = "v8py.Context";
    py_context_type.tp_basicsize = sizeof(py_context);
    py_context_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_context_type.tp_doc = "";
    py_context_type.tp_dealloc = (destructor) py_context_dealloc;
    py_context_type.tp_new = (newfunc) py_context_new;
    py_context_type.tp_methods = py_context_methods;
    py_context_type.tp_getset = py_context_getset;
    return PyType_Ready(&py_context_type);
}

void py_context_dealloc(py_context *self) {
    delete self->js_context;
    self->ob_type->tp_free((PyObject *) self);
}

PyObject *py_context_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    py_context *self = (py_context *) type->tp_alloc(type, 0);
    if (self != NULL) {
        Isolate::Scope isolate_scope(isolate);
        HandleScope handle_scope(isolate);

        Local<Context> context = Context::New(isolate);

        self->js_context = new Persistent<Context>();
        self->js_context->Reset(isolate, context);
    }
    return (PyObject *) self;
}

PyObject *py_context_eval(py_context *self, PyObject *program) {
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = self->js_context->Get(isolate);
    Context::Scope context_scope(context);

    if (PyString_CheckExact(program)) {
        program = PyUnicode_FromObject(program);
    } else {
        Py_INCREF(program);
    }

    Local<String> js_program = unwrap_local(String::NewFromTwoByte(isolate, PyUnicode_AS_UNICODE(program), NewStringType::kNormal));
    MaybeLocal<Script> script = Script::Compile(context, js_program);
    if (script.IsEmpty()) {
        PyErr_SetString(PyExc_ValueError, "javascript compilation error");
        Py_DECREF(program);
        return NULL;
    }
    MaybeLocal<Value> result = script.ToLocalChecked()->Run(context);
    if (result.IsEmpty()) {
        PyErr_SetString(PyExc_ValueError, "javascript runtime error");
        Py_DECREF(program);
        return NULL;
    }

    Py_DECREF(program);
    return py_from_js(result.ToLocalChecked(), context);
}

PyObject *py_context_get_global(py_context *self, void *shit) {
    HandleScope hs(isolate);
    Local<Context> context = self->js_context->Get(isolate);
    return py_from_js(context->Global(), context);
}

PyObject *py_context_gc(py_context *self) {
    Isolate::Scope isolate_scope(isolate);

    isolate->RequestGarbageCollectionForTesting(Isolate::GarbageCollectionType::kFullGarbageCollection);

    Py_INCREF(Py_None);
    return Py_None;
}

