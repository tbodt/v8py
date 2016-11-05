#ifndef JSOBJECT_H
#define JSOBJECT_H

#include <Python.h>
#include <v8.h>

using namespace v8;

typedef struct {
    PyObject_HEAD
    Persistent<Object> *object;
    Persistent<Context> *context;
} py_js_object;
extern PyTypeObject py_js_object_type;
int py_js_object_type_init();

py_js_object *py_js_object_new(Local<Object> object, Local<Context> context);
PyObject *py_js_object_fake_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
void py_js_object_dealloc(py_js_object *self);

PyObject *py_js_object_getattro(py_js_object *self, PyObject *name);
int py_js_object_setattro(py_js_object *self, PyObject *name, PyObject *value);
PyObject *py_js_object_dir(py_js_object *self);
PyObject *py_js_object_repr(py_js_object *self);

typedef struct {
    py_js_object object;
} py_js_function;
extern PyTypeObject py_js_function_type;
int py_js_function_type_init();

PyObject *py_js_function_call(py_js_function *self, PyObject *args, PyObject *kwargs);

#endif
