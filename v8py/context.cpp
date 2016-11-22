#include <Python.h>
#include <v8.h>
#include <pthread.h>

#include "v8py.h"
#include "context.h"
#include "convert.h"
#include "pyfunction.h"

using namespace v8;

PyMethodDef context_methods[] = {
    {"eval", (PyCFunction) context_eval, METH_VARARGS | METH_KEYWORDS, NULL},
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

        context->SetEmbedderData(OBJECT_PROTOTYPE_SLOT, Object::New(isolate)->GetPrototype());
        context->SetEmbedderData(ERROR_PROTOTYPE_SLOT, Exception::Error(String::Empty(isolate)).As<Object>()->GetPrototype());
    }
    return (PyObject *) self;
}

void *breaker_thread(void *param) {
    double timeout = *(double *) param;
    usleep((int) (timeout * 1000000));
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    isolate->TerminateExecution();
    return NULL;
}

PyObject *context_eval(context *self, PyObject *args, PyObject *kwargs) {
    PyObject *program;
    double timeout = 0;
    static char *keywords[] = {"program", "timeout", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|d", keywords, &program, &timeout) < 0) {
        return NULL;
    }
    if (!PyString_Check(program)) {
        PyErr_SetString(PyExc_TypeError, "program must be a string");
        return NULL;
    }

    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Context> context = self->js_context.Get(isolate);
    Context::Scope cs(context);
    JS_TRY

    MaybeLocal<Script> script = Script::Compile(context, js_from_py(program, context).As<String>());
    PY_PROPAGATE_JS;

    pthread_t breaker_id;
    if (timeout > 0) {
        errno = pthread_create(&breaker_id, NULL, breaker_thread, &timeout);
        if (errno) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }
    }

    MaybeLocal<Value> result = script.ToLocalChecked()->Run(context);

    if (timeout > 0) {
        pthread_cancel(breaker_id);
        errno = pthread_join(breaker_id, NULL);
        if (errno) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }
    }

    PY_PROPAGATE_JS;
    return py_from_js(result.ToLocalChecked(), context);
}

PyObject *context_get_global(context *self, void *shit) {
    Isolate::Scope is(isolate);
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

