#ifndef CONVERT_H
#define CONVERT_H

#include <Python.h>
#include <v8.h>

PyObject *py_from_js(Local<Value> js_value, Local<Context> context);
// If any Python exceptions are thrown in the process, they get swallowed.
// Because they're probably never going to be too serious. Only like
// MemoryError.
Local<Value> js_from_py(PyObject *py_value, Local<Context> context);

PyObject *pys_from_jss(const FunctionCallbackInfo<Value> &js_args, Local<Context> context);
// js_args is an out parameter, expected to contain enough space
void jss_from_pys(PyObject *py_args, Local<Value> *js_args, Local<Context> context);

#endif
