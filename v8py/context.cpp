#include <Python.h>
#include <v8.h>

#include "context.h"
#include "pyfunction.h"
#include "convert.h"
#include "v8py.h"

using namespace v8;

PyMethodDef context_methods[] = {
    {"eval", (PyCFunction) context_eval, METH_O, NULL},
    {"gc", (PyCFunction) context_gc, METH_NOARGS, NULL},
    {NULL},
};
// Python is wrong. The first entry is not modifiable and should be const char *
PyGetSetDef context_getset[] = {
    {(char *) "glob", (getter) context_get_global, NULL, NULL, NULL},
    {NULL},
};
PyMappingMethods context_mapping = {
    NULL, (binaryfunc) context_getitem, (objobjargproc) context_setitem
};
PyTypeObject context_type = {
    PyObject_HEAD_INIT(NULL)
};
int context_type_init() {
    context_type.tp_name = "v8py.Context";
    context_type.tp_basicsize = sizeof(context);
    context_type.tp_flags = Py_TPFLAGS_DEFAULT;
    context_type.tp_doc = "";
    context_type.tp_dealloc = (destructor) context_dealloc;
    context_type.tp_new = (newfunc) context_new;
    context_type.tp_methods = context_methods;
    context_type.tp_getattro = (getattrofunc) context_getattro;
    context_type.tp_setattro = (setattrofunc) context_setattro;
    context_type.tp_getset = context_getset;
    context_type.tp_as_mapping = &context_mapping;
    return PyType_Ready(&context_type);
}

void context_dealloc(context *self) {
    self->js_context.Reset();
    self->ob_type->tp_free((PyObject *) self);
}

PyObject *context_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    context *self = (context *) type->tp_alloc(type, 0);
    if (self != NULL) {
        Isolate::Scope isolate_scope(isolate);
        HandleScope handle_scope(isolate);

        Local<Context> context = Context::New(isolate);
        Context::Scope cs(context);
        self->js_context.Reset(isolate, context);
        context->SetEmbedderData(1, Object::New(isolate)->GetPrototype());
    }
    return (PyObject *) self;
}

PyObject *context_eval(context *self, PyObject *program) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Context> context = self->js_context.Get(isolate);
    Context::Scope cs(context);

    MaybeLocal<Script> script = Script::Compile(context, js_from_py(program, context).As<String>());
    if (script.IsEmpty()) {
        PyErr_SetString(PyExc_ValueError, "JavaScript compilation error");
        return NULL;
    }
    MaybeLocal<Value> result = script.ToLocalChecked()->Run(context);
    if (result.IsEmpty()) {
        PyErr_SetString(PyExc_ValueError, "JavaScript runtime error");
        return NULL;
    }

    return py_from_js(result.ToLocalChecked(), context);
}

PyObject *context_get_global(context *self, void *shit) {
    HandleScope hs(isolate);
    Local<Context> context = self->js_context.Get(isolate);
    return py_from_js(context->Global(), context);
}

PyObject *context_getattro(context *self, PyObject *name) {
    PyObject *value = PyObject_GenericGetAttr((PyObject *) self, name);
    if (value == NULL) {
        PyErr_Clear();
        return context_getitem(self, name);
    }
    return value;
}
PyObject *context_getitem(context *self, PyObject *name) {
    PyObject *global = context_get_global(self, NULL);
    PyErr_PROPAGATE(global);
    return PyObject_GetAttr(context_get_global(self, NULL), name);
}

int context_setattro(context *self, PyObject *name, PyObject *value) {
    // use GenericGetAttr to find out if Context defines the property
    PyObject *ctx_value = PyObject_GenericGetAttr((PyObject *) self, name);
    if (ctx_value == NULL) {
        // if the property is not defined by Context, set it on the global
        PyErr_Clear();
        return context_setitem(self, name, value);
    } else {
        Py_DECREF(ctx_value);
    }
    // if the property is defined by Context, delegate
    return PyObject_GenericSetAttr((PyObject *) self, name, value);
}
int context_setitem(context *self, PyObject *name, PyObject *value) {
    PyObject *global = context_get_global(self, NULL);
    PyErr_PROPAGATE_(global);
    return PyObject_SetAttr(global, name, value);
}

PyObject *context_gc(context *self) {
    Isolate::Scope isolate_scope(isolate);

    isolate->RequestGarbageCollectionForTesting(Isolate::GarbageCollectionType::kFullGarbageCollection);

    Py_RETURN_NONE;
}

