#ifndef CONTEXT_H
#define CONTEXT_H

#include <Python.h>
#include <v8.h>

#include "template.h"

using namespace v8;

typedef struct {
    PyObject_HEAD
    Persistent<Context> *js_context;
} py_context;
int py_context_type_init();

void py_context_dealloc(py_context *self);
PyObject *py_context_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
PyObject *py_context_eval(py_context *self, PyObject *program);
PyObject *py_context_gc(py_context *self);

PyObject *py_context_get_global(py_context *self, void *shit);

PyObject *py_context_getattro(py_context *self, PyObject *name);
PyObject *py_context_getitem(py_context *self, PyObject *name);
int py_context_setattro(py_context *self, PyObject *name, PyObject *value);
int py_context_setitem(py_context *self, PyObject *name, PyObject *value);

extern PyTypeObject py_context_type;

#endif
