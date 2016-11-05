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
PyObject *py_template_new(PyObject *func);

PyObject *py_function_to_template(PyObject *func);
Local<Function> py_template_to_function(py_template *self, Local<Context> context);

extern PyTypeObject py_template_type;

#endif
