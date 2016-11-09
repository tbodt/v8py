#ifndef CONTEXT_H
#define CONTEXT_H

#include <Python.h>
#include <v8.h>

#include "template.h"

using namespace v8;

typedef struct {
    PyObject_HEAD
    Persistent<Context> *js_context;
} context;
int context_type_init();

void context_dealloc(context *self);
PyObject *context_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
PyObject *context_eval(context *self, PyObject *program);
PyObject *context_gc(context *self);

PyObject *context_get_global(context *self, void *shit);

PyObject *context_getattro(context *self, PyObject *name);
PyObject *context_getitem(context *self, PyObject *name);
int context_setattro(context *self, PyObject *name, PyObject *value);
int context_setitem(context *self, PyObject *name, PyObject *value);

extern PyTypeObject context_type;

#endif
