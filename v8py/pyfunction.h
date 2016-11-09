#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <Python.h>
#include <v8.h>

using namespace v8;

typedef struct {
    PyObject_HEAD
    PyObject *function;
    PyObject *function_name;
    Persistent<FunctionTemplate> *js_template;
} py_function;
int py_function_type_init();

void py_function_dealloc(py_function *self);
PyObject *py_function_new(PyObject *func);

PyObject *py_function_to_template(PyObject *func);
Local<Function> py_template_to_function(py_function *self, Local<Context> context);

extern PyTypeObject py_function_type;

#endif
