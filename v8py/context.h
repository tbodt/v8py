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
PyObject *py_context_add_template(py_context *self, PyObject *args);
PyObject *py_context_gc(py_context *self);

PyObject *py_context_get_global(py_context *self, void *shit);

extern PyTypeObject py_context_type;

#endif
