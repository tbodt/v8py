#ifndef JSOBJECT_H
#define JSOBJECT_H

#include <Python.h>
#include <v8.h>

using namespace v8;

typedef struct {
    PyObject_HEAD
    Persistent<Object> object;
    Persistent<Context> context;
} js_object;
extern PyTypeObject js_object_type;
int js_object_type_init();

js_object *js_object_new(Local<Object> object, Local<Context> context);
PyObject *js_object_fake_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
void js_object_dealloc(js_object *self);

PyObject *js_object_getattro(js_object *self, PyObject *name);
int js_object_setattro(js_object *self, PyObject *name, PyObject *value);
Py_ssize_t js_object_length(js_object *self);
PyObject *js_object_dir(js_object *self);
PyObject *js_object_repr(js_object *self);


typedef struct {
    js_object object;
    Persistent<Value> js_this;
} js_function;
extern PyTypeObject js_function_type;
int js_function_type_init();

PyObject *js_function_call(js_function *self, PyObject *args, PyObject *kwargs);

#endif
