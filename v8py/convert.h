#ifndef CONVERT_H
#define CONVERT_H

#include <Python.h>
#include <v8.h>

PyObject *py_from_js(Local<Value> js_value, Local<Context> context);
Local<Value> js_from_py(PyObject *py_value);

#endif
