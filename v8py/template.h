#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <Python.h>
#include <v8.h>

using namespace v8;

typedef struct {
    PyObject_HEAD
    PyObject *function;
    Persistent<FunctionTemplate> *js_template;
} py_template;
int py_template_type_init();

void py_template_dealloc(py_template *self);
PyObject *py_template_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
PyObject *py_template_init(py_template *self, PyObject *args, PyObject *kwargs);

Local<Function> py_template_to_function(py_template *self, Local<Context> context);

extern PyTypeObject py_template_type;

#endif
